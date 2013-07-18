#include "nef/primitives.h"
#include "nef/io.h"
#include "nef/translate.h"
#include "nef/ops.h"
#include <iostream>
#include <fstream>

int main()
{
	using namespace nef;
	
	auto shank = make_cone(0, 0, 60, 0, 0, 28, 10, 10, 64);
	auto flutes = make_cone(0, 0, 28, 0, 0, 0, 10, 10, 64);
	const auto tool = shank + flutes;
	
	const auto stock = make_box(0, 0, 0, 50, 50, 100);
	
	{
		auto t0 = rotate(tool, 0.707107, 0.707107, 0, 0);
		auto t00 = translate(t0, 50, 0, 90);

		auto s0 = stock - t00;
		
		std::ofstream os("nef_nonmanifold-s0.off");
		write_off(os, s0);
	}
	
	{
		auto t1 = rotate(tool, 0.718126, 0.695913, 0, 0);
		auto t01 = translate(t1, 50, 0, 90);
		
		auto s1 = stock - t01;

		std::ofstream os("nef_nonmanifold-s1.off");
		write_off(os, s1);
	}
	
	{
		auto t0 = rotate(tool, 0.707107, 0.707107, 0, 0);

		polyline_t path{ { {50, 0, 90}, {49, 0, 90} } };
		auto tool_path = glide(t0, path);
		
		auto s2 = stock - tool_path;
		
		std::ofstream os("nef_nonmanifold-s2.off");
		write_off(os, s2);
	}
}

