/* cxxcam - C++ CAD/CAM driver library.
 * Copyright (C) 2013  Nicholas Gill
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Machine.cpp
 *
 *  Created on: 26/04/2012
 *      Author: nicholas
 */

#include "Machine.h"
#include "MachineState.h"
#include "GCode.h"
#include "Axis.h"
#include "Tool.h"
#include "ToolTable.h"
#include "Stock.h"
#include "Spindle.h"

#include <iostream>
#include <sstream>
#include <stdexcept>

namespace
{
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
}

using gcode::Word;
using gcode::Line;
using gcode::Block;
using gcode::Code;

struct Machine::Private
{
	const Type m_Type;

	MachineState m_State;
	Code m_GCode;
	Spindle m_Spindle;
	ToolTable m_ToolTable;
	Stock m_Stock;

	Private(Type type, const std::string& gcode_variant);
};

Machine::Private::Private(Type type, const std::string& gcode_variant)
 : 	m_Type(type), m_State(), m_GCode(gcode_variant), m_Spindle(), m_ToolTable(), m_Stock()
{
}

const Word Machine::G00(Word::G, 0);
const Word Machine::G01(Word::G, 1);
const Word Machine::G17(Word::G, 17);
const Word Machine::G18(Word::G, 18);
const Word Machine::G19(Word::G, 19);
const Word Machine::G17_1(Word::G, 17.1);
const Word Machine::G18_1(Word::G, 18.1);
const Word Machine::G19_1(Word::G, 19.1);
const Word Machine::G20(Word::G, 20);
const Word Machine::G21(Word::G, 21);
const Word Machine::G40(Word::G, 40);
const Word Machine::G49(Word::G, 49);
const Word Machine::G54(Word::G, 54);

const Word Machine::G61(Word::G, 61);
const Word Machine::G61_1(Word::G, 61.1);
const Word Machine::G64(Word::G, 64);

const Word Machine::G80(Word::G, 80);
const Word Machine::G90(Word::G, 90);
const Word Machine::G90_1(Word::G, 90.1);
const Word Machine::G91(Word::G, 91);
const Word Machine::G91_1(Word::G, 91.1);
const Word Machine::G93(Word::G, 93);
const Word Machine::G94(Word::G, 94);
const Word Machine::G95(Word::G, 95);
const Word Machine::G97(Word::G, 97);

const Word Machine::M01(Word::M, 1);
const Word Machine::M02(Word::M, 2);
const Word Machine::M03(Word::M, 3);
const Word Machine::M04(Word::M, 4);
const Word Machine::M05(Word::M, 5);
const Word Machine::M06(Word::M, 6);
const Word Machine::M09(Word::M, 9);

void Machine::Preamble()
{
	const auto& m_Type = m_Private->m_Type;
	const auto& m_State = m_Private->m_State;
	auto& m_GCode = m_Private->m_GCode;

	std::stringstream c;
	c << "cxxcam  ";

	switch(m_Type)
	{
		case Type::Mill:
			c << "Mill  ";
			break;
		case Type::Lathe:
			c << "Lathe  ";
			break;
	}

	Line line;

	line += G00;

	switch(m_State.m_Plane)
	{
		case Plane::XY:
			c << "XY  ";
			line += G17;
			break;
		case Plane::ZX:
			c << "ZX  ";
			line += G18;
			break;
		case Plane::YZ:
			c << "YZ  ";
			line += G19;
			break;
		case Plane::UV:
			c << "UV  ";
			line += G17_1;
			break;
		case Plane::WU:
			c << "WU  ";
			line += G18_1;
			break;
		case Plane::VW:
			c << "VW  ";
			line += G19_1;
			break;
	}

	switch(m_State.m_Units)
	{
		case Units::Metric:
			c << "Metric  ";
			line += G21;
			break;
		case Units::Imperial:
			c << "Imperial  ";
			line += G20;
			break;
	}

	// Cancel Cutter Compensation
	line += G40;

	// Cancel Tool Length Compensation
	line += G49;

	// Select coordinate system 1
	line += G54;

	// Cancel canned cycles
	line += G80;

	switch(m_State.m_Motion)
	{
		case Motion::Absolute:
			c << "Absolute  ";
			line += G90;
			break;
		case Motion::Incremental:
			c << "Incremental  ";
			line += G91;
			break;
	}

	switch(m_State.m_ArcMotion)
	{
		case Motion::Absolute:
			c << "Absolute Arc  ";
			line += G90_1;
			break;
		case Motion::Incremental:
			c << "Incremental Arc  ";
			line += G91_1;
			break;
	}

	switch(m_State.m_FeedRateMode)
	{
		case FeedRateMode::InverseTime:
			c << "Inverse Time  ";
			line += G93;
			break;
		case FeedRateMode::UnitsPerMinute:
			c << "Units Per Minute  ";
			line += G94;
			break;
		case FeedRateMode::UnitsPerRevolution:
			c << "Units Per Revolution  ";
			line += G95;
			break;
	}

	// Spindle speed is in RPM
	line += G97;

	// Coolant off
	line += M09;

	// Spindle off
	line += M05;

	m_GCode.NewBlock(c.str(), m_State);
	m_GCode.AddLine(line);
	m_GCode.EndBlock();
}

Word Machine::AxisToWord(const Axis& axis)
{
	switch(axis)
	{
		case Axis::Type::X:
			return {Word::X, axis};
		case Axis::Type::Y:
			return {Word::Y, axis};
		case Axis::Type::Z:
			return {Word::Z, axis};

		case Axis::Type::A:
			return {Word::A, axis};
		case Axis::Type::B:
			return {Word::B, axis};
		case Axis::Type::C:
			return {Word::C, axis};

		case Axis::Type::U:
			return {Word::U, axis};
		case Axis::Type::V:
			return {Word::V, axis};
		case Axis::Type::W:
			return {Word::W, axis};
	}

	throw std::logic_error("Unknown Axis.");
}

double Machine::MillFeedRate(double chip_load, int flutes, double spindle_speed)
{
	return chip_load * flutes * spindle_speed;
}

double Machine::MillSpindleSpeed(double cutting_speed, double cutter_diameter)
{
	return cutting_speed / (3.14159 * cutter_diameter);
}

void Machine::UpdatePosition(const Axis& axis)
{
	auto& m_State = m_Private->m_State;

	double* val = nullptr;
	switch(axis)
	{
		case Axis::Type::X:
			val = &m_State.m_Current.X;
			break;
		case Axis::Type::Y:
			val = &m_State.m_Current.Y;
			break;
		case Axis::Type::Z:
			val = &m_State.m_Current.Z;
			break;

		case Axis::Type::A:
			val = &m_State.m_Current.A;
			break;
		case Axis::Type::B:
			val = &m_State.m_Current.B;
			break;
		case Axis::Type::C:
			val = &m_State.m_Current.C;
			break;

		case Axis::Type::U:
			val = &m_State.m_Current.U;
			break;
		case Axis::Type::V:
			val = &m_State.m_Current.V;
			break;
		case Axis::Type::W:
			val = &m_State.m_Current.W;
			break;
	}

	if(!val)
		throw std::logic_error("Unknown Axis");

	switch(m_State.m_Motion)
	{
		case Motion::Absolute:
			*val = axis;
			break;
		case Motion::Incremental:
			*val += axis;
			break;
	}
}

Machine::Machine(Type type, const std::string& gcode_variant)
 : m_Private(make_unique<Private>(type, gcode_variant))
{
	const auto& m_Type = m_Private->m_Type;
	auto& m_State = m_Private->m_State;

	switch(m_Type)
	{
		case Type::Mill:
			m_State.m_Plane = Plane::XY;
			m_State.m_FeedRateMode = FeedRateMode::UnitsPerMinute;
			break;
		case Type::Lathe:
			m_State.m_Plane = Plane::ZX;
			m_State.m_FeedRateMode = FeedRateMode::UnitsPerRevolution;
			break;
	}

	Preamble();
}
Machine::Machine(const Machine& m)
 : m_Private(make_unique<Private>(*m.m_Private))
{
}
Machine& Machine::operator=(const Machine& m)
{
	m_Private = make_unique<Private>(*m.m_Private);
	return *this;
}

void Machine::dump() const
{
	auto& m_State = m_Private->m_State;

	if(m_State.m_SpindleRotation != Rotation::Stop)
		std::cerr << m_State.m_SpindleSpeed << " RPM " << (m_State.m_SpindleRotation == Rotation::Clockwise ? "Clockwise" : "Counter-Clockwise") << "\n";
	std::cerr << m_State.m_Current.str() << "\n";
}

bool Machine::AddTool(int id, const Tool& tool)
{
	const auto& m_Type = m_Private->m_Type;
	switch(m_Type)
	{
		case Type::Mill:
			if(tool.ToolType() != Tool::Type::Mill)
				throw std::logic_error("Must use Mill tool with Mill.");
			break;
		case Type::Lathe:
			if(tool.ToolType() != Tool::Type::Lathe)
				throw std::logic_error("Must use Lathe tool with Lathe.");
			break;
	}
	return m_Private->m_ToolTable.AddTool(id, tool);
}
bool Machine::RemoveTool(int id)
{
	return m_Private->m_ToolTable.RemoveTool(id);
}
void Machine::AddSpindleRange(unsigned long range_start, unsigned long range_end)
{
	m_Private->m_Spindle.AddRange(range_start, range_end);
}
void Machine::AddSpindleDiscrete(unsigned long discrete_value)
{
	m_Private->m_Spindle.AddDiscrete(discrete_value);
}
void Machine::SetStock(const Stock& stock)
{
	m_Private->m_Stock = stock;
}

void Machine::SetTool(int id)
{
	auto& m_GCode = m_Private->m_GCode;
	auto& m_ToolTable = m_Private->m_ToolTable;

	Tool tool;
	if(id && m_ToolTable.Get(id, &tool))
	{
		m_GCode.AddLine(Line(Word(Word::T, id), "Preload tool " + tool.Name()));
	}
	else if(id == 0)
	{
		m_GCode.AddLine(Line(Word(Word::T, id), "Preload empty tool"));
	}
	else
	{
		// Unknown tool.
		std::stringstream s;
		s << "Preload Unknown tool id: " << id;
		throw std::logic_error(s.str());
	}
}
void Machine::ToolChange(int id)
{
	auto& m_State = m_Private->m_State;
	auto& m_GCode = m_Private->m_GCode;
	auto& m_ToolTable = m_Private->m_ToolTable;

	if(m_State.m_CurrentTool != id)
	{
		Tool tool;
		if(id && m_ToolTable.Get(id, &tool))
		{
			m_State.m_CurrentTool = id;

			Line line;
			line.Comment("Switch to tool " + tool.Name());
			line += Word(Word::T, id);
			line += M06;
			m_GCode.AddLine(line);
		}
		else if(id == 0)
		{
			m_State.m_CurrentTool = id;

			Line line;
			line.Comment("Empty Spindle");
			line += Word(Word::T, 0);
			line += M06;
			m_GCode.AddLine(line);
		}
		else
		{
			// Unknown tool.
			std::stringstream s;
			s << "Unknown tool id: " << id;
			throw std::logic_error(s.str());
		}
	}
}

void Machine::NewBlock(const std::string& name)
{
	m_Private->m_GCode.NewBlock(name, m_Private->m_State);
}
void Machine::EndBlock(int restore)
{
	auto& m_State = m_Private->m_State;
	auto& m_GCode = m_Private->m_GCode;

	if(restore)
	{
		m_GCode.AddLine(Line("Restore State"));

		Block& block = m_GCode.CurrentBlock();
		const MachineState& saved_state = block.State();

		if(restore & block_RestoreUnits)
			SetUnits(saved_state.m_Units);
		if(restore & block_RestorePlane)
			SetPlane(saved_state.m_Plane);
		if(restore & block_RestoreMotion)
			SetMotion(saved_state.m_Motion);
		if(restore & block_RestoreArcMotion)
			SetArcMotion(saved_state.m_ArcMotion);
		if(restore & block_RestoreFeedRateMode)
			SetFeedRateMode(saved_state.m_FeedRateMode);
		if(restore & block_RestoreFeedRate)
			SetFeedRate(saved_state.m_FeedRate);
		if(restore & block_RestoreSpindle)
			StartSpindle(saved_state.m_SpindleSpeed, saved_state.m_SpindleRotation);
		if(restore & block_RestoreTool)
			ToolChange(saved_state.m_CurrentTool);
		if(restore & block_RestorePosition)
		{
			if(m_State.m_Current != saved_state.m_Current)
			{
				// TODO determine how to safely rapid to the previous position.
			}
		}
	}
	m_GCode.EndBlock();
}

void Machine::AccuracyExactPath()
{
	m_Private->m_GCode.AddLine(Line(G61, "Exact Path"));
}
void Machine::AccuracyExactStop()
{
	m_Private->m_GCode.AddLine(Line(G61_1, "Exact Stop"));
}
void Machine::AccuracyPathBlending()
{
	m_Private->m_GCode.AddLine(Line(G64, "Path Blend Without Tolerance"));
}
void Machine::AccuracyPathBlending(double p)
{
	Line line(G64, "Path Blend With Tolerance");
	line += Word(Word::P, p);
	m_Private->m_GCode.AddLine(line);
}
void Machine::AccuracyPathBlending(double p, double q)
{
	Line line(G64, "Path Blend With Tolerance & Folding");
	line += Word(Word::P, p);
	line += Word(Word::Q, q);
	m_Private->m_GCode.AddLine(line);
}

void Machine::OptionalPause(const std::string& comment)
{
	m_Private->m_GCode.AddLine(Line(M01, comment));
}

void Machine::SetMotion(Motion m)
{
	auto& m_State = m_Private->m_State;
	auto& m_GCode = m_Private->m_GCode;

	if(m_State.m_Motion != m)
	{
		m_State.m_Motion = m;

		switch(m_State.m_Motion)
		{
			case Motion::Absolute:
				m_GCode.AddLine(Line(G90, "Switch to Absolute Motion"));
				break;
			case Motion::Incremental:
				m_GCode.AddLine(Line(G91, "Switch to Incremental Motion"));
				break;
		}
	}
}
void Machine::SetArcMotion(Motion m)
{
	auto& m_State = m_Private->m_State;
	auto& m_GCode = m_Private->m_GCode;

	if(m_State.m_ArcMotion != m)
	{
		m_State.m_ArcMotion = m;

		switch(m_State.m_ArcMotion)
		{
			case Motion::Absolute:
				m_GCode.AddLine(Line(G90_1, "Switch to Absolute Arc Motion"));
				break;
			case Motion::Incremental:
				m_GCode.AddLine(Line(G91_1, "Switch to Incremental Arc Motion"));
				break;
		}
	}
}
void Machine::SetUnits(Units u)
{
	auto& m_State = m_Private->m_State;
	auto& m_GCode = m_Private->m_GCode;

	if(m_State.m_Units != u)
	{
		m_State.m_Units = u;

		switch(m_State.m_Units)
		{
			case Units::Metric:
				m_GCode.AddLine(Line(G21, "Switch to Metric (Millimeters)"));
				break;
			case Units::Imperial:
				m_GCode.AddLine(Line(G20, "Switch to Imperial (Inches)"));
				break;
		}

		if(m_State.m_FeedRate > 0.0)
		{
			std::stringstream c;
			c << "Active feed rate meaning changed to ";

			switch(m_State.m_FeedRateMode)
			{
				case FeedRateMode::InverseTime:
					c << "Feed Time: " << 1/m_State.m_FeedRate << " minutes";
					break;
				case FeedRateMode::UnitsPerMinute:
					switch(m_State.m_Units)
					{
						case Units::Metric:
							c << m_State.m_FeedRate << "mm per minute";
							break;
						case Units::Imperial:
							c << m_State.m_FeedRate << "\" per minute";
							break;
					}
					break;
				case FeedRateMode::UnitsPerRevolution:
					switch(m_State.m_Units)
					{
						case Units::Metric:
							c << m_State.m_FeedRate << "mm per revolution";
							break;
						case Units::Imperial:
							c << m_State.m_FeedRate << "\" per revolution";
							break;
					}
					break;
			}
			m_GCode.AddLine(Line(c.str()));
		}
	}
}
void Machine::SetPlane(Plane p)
{
	auto& m_State = m_Private->m_State;
	auto& m_GCode = m_Private->m_GCode;

	if(m_State.m_Plane != p)
	{
		m_State.m_Plane = p;

		switch(m_State.m_Plane)
		{
			case Plane::XY:
				m_GCode.AddLine(Line(G17, "Switch to XY Plane"));
				break;
			case Plane::ZX:
				m_GCode.AddLine(Line(G18, "Switch to ZX Plane"));
				break;
			case Plane::YZ:
				m_GCode.AddLine(Line(G19, "Switch to YZ Plane"));
				break;
			case Plane::UV:
				m_GCode.AddLine(Line(G17_1, "Switch to UV Plane"));
				break;
			case Plane::WU:
				m_GCode.AddLine(Line(G18_1, "Switch to WU Plane"));
				break;
			case Plane::VW:
				m_GCode.AddLine(Line(G19_1, "Switch to VW Plane"));
				break;
		}
	}
}
void Machine::SetFeedRateMode(FeedRateMode f)
{
	auto& m_State = m_Private->m_State;
	auto& m_GCode = m_Private->m_GCode;

	if(m_State.m_FeedRateMode != f)
	{
		m_State.m_FeedRateMode = f;

		switch(m_State.m_FeedRateMode)
		{
			case FeedRateMode::InverseTime:
				m_GCode.AddLine(Line(G93, "Switch to Inverse Time Feed Rate Mode"));
				break;
			case FeedRateMode::UnitsPerMinute:
				m_GCode.AddLine(Line(G94, "Switch to Units Per Minute Feed Rate Mode"));
				break;
			case FeedRateMode::UnitsPerRevolution:
				m_GCode.AddLine(Line(G95, "Switch to Units Per Revolution Feed Rate Mode"));
				break;
		}

		if(m_State.m_FeedRate > 0.0)
		{
			std::stringstream c;
			c << "Active feed rate meaning changed to ";

			switch(m_State.m_FeedRateMode)
			{
				case FeedRateMode::InverseTime:
					c << "Feed Time: " << 1/m_State.m_FeedRate << " minutes";
					break;
				case FeedRateMode::UnitsPerMinute:
					switch(m_State.m_Units)
					{
						case Units::Metric:
							c << m_State.m_FeedRate << "mm per minute";
							break;
						case Units::Imperial:
							c << m_State.m_FeedRate << "\" per minute";
							break;
					}
					break;
				case FeedRateMode::UnitsPerRevolution:
					switch(m_State.m_Units)
					{
						case Units::Metric:
							c << m_State.m_FeedRate << "mm per revolution";
							break;
						case Units::Imperial:
							c << m_State.m_FeedRate << "\" per revolution";
							break;
					}
					break;
			}
			m_GCode.AddLine(Line(c.str()));
		}
	}
}
void Machine::SetFeedRate(double f)
{
	auto& m_State = m_Private->m_State;
	auto& m_GCode = m_Private->m_GCode;

	if(m_State.m_FeedRate != f)
	{
		m_State.m_FeedRate = f;

		std::stringstream c;
		if(f > 0.0)
		{
			switch(m_State.m_FeedRateMode)
			{
				case FeedRateMode::InverseTime:
					c << "Feed Time: " << 1/f << " minutes";
					break;
				case FeedRateMode::UnitsPerMinute:
					switch(m_State.m_Units)
					{
						case Units::Metric:
							c << f << "mm per minute";
							break;
						case Units::Imperial:
							c << f << "\" per minute";
							break;
					}
					break;
				case FeedRateMode::UnitsPerRevolution:
					switch(m_State.m_Units)
					{
						case Units::Metric:
							c << f << "mm per revolution";
							break;
						case Units::Imperial:
							c << f << "\" per revolution";
							break;
					}
					break;
			}
		}

		Line line(c.str());
		if(m_State.m_FeedRateMode != FeedRateMode::InverseTime)
			line += Word(Word::F, f);
		m_GCode.AddLine(line);
	}
}
void Machine::StartSpindle(unsigned long s, Rotation r)
{
	auto& m_State = m_Private->m_State;
	auto& m_GCode = m_Private->m_GCode;
	auto& m_Spindle = m_Private->m_Spindle;

	unsigned long requested_speed(s);
	s = m_Spindle.Normalise(s);

	if(m_State.m_SpindleSpeed != s || m_State.m_SpindleRotation != r)
	{
		m_State.m_SpindleSpeed = s;
		m_State.m_SpindleRotation = r;

		Line line;
		std::stringstream c;
		switch(m_State.m_SpindleRotation)
		{
			case Rotation::Stop:
				m_State.m_SpindleSpeed = 0;
				line += M05;
				c << "Stop Spindle";
				break;
			case Rotation::Clockwise:
				line += M03;
				line += Word(Word::S, s);
				c << "Start Spindle Clockwise " << s << " RPM";
				if(m_State.m_SpindleSpeed != requested_speed)
					c << " (" << requested_speed << " RPM Requested)";
				break;
			case Rotation::CounterClockwise:
				line += M04;
				line += Word(Word::S, s);
				c << "Start Spindle Counter Clockwise " << s << " RPM";
				if(m_State.m_SpindleSpeed != requested_speed)
					c << " (" << requested_speed << " RPM Requested)";
				break;
		}
		line.Comment(c.str());
		m_GCode.AddLine(line);
	}
}
void Machine::StopSpindle()
{
	auto& m_State = m_Private->m_State;
	auto& m_GCode = m_Private->m_GCode;

	if(m_State.m_SpindleRotation != Rotation::Stop || m_State.m_SpindleSpeed > 0)
	{
		m_State.m_SpindleSpeed = 0;
		m_State.m_SpindleRotation = Rotation::Stop;

		m_GCode.AddLine(Line(M05, "Stop Spindle"));
	}
}

void Machine::Rapid(const std::vector<Axis>& axes)
{
	Line line;

	line += G00;
	for(auto& axis : axes)
	{
		line += AxisToWord(axis);
		UpdatePosition(axis);
	}
	m_Private->m_GCode.AddLine(line);
}

void Machine::Linear(const std::vector<Axis>& axes)
{
	auto& m_State = m_Private->m_State;

	if(m_State.m_SpindleRotation == Rotation::Stop)
		throw std::logic_error("Spindle is stopped");
	if(m_State.m_FeedRate == 0.0)
		throw std::logic_error("Feedrate is 0.0");

	Line line;

	line += G01;
	for(auto& axis : axes)
	{
		line += AxisToWord(axis);
		UpdatePosition(axis);
	}

	if(m_State.m_FeedRateMode == FeedRateMode::InverseTime)
	{
		std::stringstream c;
		c << "Feed Time: " << 1/m_State.m_FeedRate << " minutes";
		line += Word(Word::F, m_State.m_FeedRate, c.str());
	}

	m_Private->m_GCode.AddLine(line);
}

void Machine::Arc(Direction dir, Axis helix)
{
	auto& m_State = m_Private->m_State;

	if(m_State.m_SpindleRotation == Rotation::Stop)
		throw std::logic_error("Spindle is stopped");
	if(m_State.m_FeedRate == 0.0)
		throw std::logic_error("Feedrate is 0.0");
	
	switch(m_State.m_Plane)
	{
		case Plane::XY:
			if(helix != Axis::Type::Z)
				throw std::logic_error("In XY Plane helix axis must be Z");
			// G2 or G3 <X- Y- Z- I- J- P->
			// Z - helix
			// I - X offset
			// J - Y offset
			// P - number of turns
			break;
		case Plane::ZX:
			if(helix != Axis::Type::Y)
				throw std::logic_error("In ZX Plane helix axis must be Y");
			// G2 or G3 <X- Z- Y- I- K- P->
			// Y - helix
			// I - X offset
			// K - Z offset
			// P - number of turns
			break;
		case Plane::YZ:
			if(helix != Axis::Type::X)
				throw std::logic_error("In YZ Plane helix axis must be X");
			// G2 or G3 <Y- Z- X- J- K- P->
			// X - helix
			// J - Y offset
			// K - Z offset
			// P - number of turns
			break;
		case Plane::UV:
		case Plane::WU:
		case Plane::VW:
			throw std::logic_error("Arc defined only on Planes XY, ZX, YZ");
			break;
	}
}

Machine::~Machine()
{
	m_Private->m_GCode.AddLine(Line(M02, "End of program."));

	std::cout << m_Private->m_GCode;
	
	m_Private->m_Stock.Write(std::cout, Stock::Format::OFF);
}

