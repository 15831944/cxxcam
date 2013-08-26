   /************************************************************************

       Copyright 2008 Mark Pictor

   This file is part of RS274NGC.

   RS274NGC is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   RS274NGC is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with RS274NGC.  If not, see <http://www.gnu.org/licenses/>.

   This software is based on software that was produced by the National
   Institute of Standards and Technology (NIST).

   ************************************************************************/

/*

Significant modifications by Nicholas Gill.
Slight restructure then plan to rip out gcode parsing for
post processor tool. Current design discards the raw block
when interpreting the high level motion etc. (which in intentional).

Plan to reuse gcode parsing and interpreter individually.

*/

#ifndef RS274NGC_HH
#define RS274NGC_HH
#include "block.h"

   /*
     rs274ngc.hh

   Declarations for the rs274abc translator.

   */

   /**********************/
   /* INCLUDE DIRECTIVES */
   /**********************/

//#include "canon.hh"
#include <cstddef>
#include <exception>

   /**********************/
   /*   COMPILER MACROS  */
   /**********************/

#define RS274NGC_TEXT_SIZE 256

   // array sizes
#define RS274NGC_ACTIVE_G_CODES 12
#define RS274NGC_ACTIVE_M_CODES 7
#define RS274NGC_ACTIVE_SETTINGS 3

   // number of parameters in parameter table
#define RS274NGC_MAX_PARAMETERS 5400

enum class Plane
{
	XY,
	YZ,
	XZ
};

enum class Units
{
	Imperial,
	Metric
};

enum class Motion
{
	Exact_Stop,
	Exact_Path,
	Continious
};

enum class SpeedFeedMode
{
	Synched,
	Independant
};

enum class Direction
{
	Stop,
	Clockwise,
	CounterClockwise
};

enum class FeedReference
{
	Workpiece,
	XYZ
};

enum class Side
{
	Right,
	Left,
	Off
};

enum class Axis
{
	X, Y, Z,
	A, B, C
};

#define CANON_TOOL_MAX 128                        // max size of carousel handled
#define CANON_TOOL_ENTRY_LEN 256                  // how long each file line can be

struct Tool
{
    int id;
    double length;
    double diameter;
    
    Tool()
     : id(), length(), diameter()
    {
    }
};

struct Position
{
	double x;
	double y;
	double z;
	double a;
	double b;
	double c;
	
	Position()
	 : x(), y(), z(), a(), b(), c()
	{}
	Position(double x, double y, double z, double a, double b, double c)
	 : x(x), y(y), z(z), a(a), b(b), c(c)
	{}
	Position(double x, double y, double z)
	 : x(x), y(y), z(z), a(), b(), c()
	{}
};

   /**********************/
   /*      TYPEDEFS      */
   /**********************/

   /* distance_mode */
enum DISTANCE_MODE {MODE_ABSOLUTE, MODE_INCREMENTAL};

   /* retract_mode for cycles */
enum RETRACT_MODE {R_PLANE, OLD_Z};

   // on-off switch settings
enum ON_OFF : bool
{
	OFF = false,
	ON = true
};

   /*

   The current_x, current_y, and current_z are the location of the tool
   in the current coordinate system. current_x and current_y differ from
   program_x and program_y when cutter radius compensation is on.
   current_z is the position of the tool tip in program coordinates when
   tool length compensation is using the actual tool length; it is the
   position of the spindle when tool length is zero.

   In a setup, the axis_offset values are set by g92 and the origin_offset
   values are set by g54 - g59.3. The net origin offset uses both values
   and is not represented here

   */

struct setup_t
{
	setup_t()
	{
		blocktext[0] = 0;
		line_length = 0;
		linetext[0] = 0;
	}

    Position axis_offset; // g92offset
    Position current;
    Position origin_offset;
    
    int active_g_codes [RS274NGC_ACTIVE_G_CODES];                // array of active G codes
    int active_m_codes [RS274NGC_ACTIVE_M_CODES];                // array of active M codes
    double active_settings [RS274NGC_ACTIVE_SETTINGS];               // array of feed, speed, etc.
    block_t block1;                                 // parsed next block
    char blocktext[RS274NGC_TEXT_SIZE];           // linetext downcased, white space gone
    Motion control_mode;               // exact path or cutting mode
    int current_slot;                             // carousel slot number of current tool
    double cutter_comp_radius;                    // current cutter compensation radius
    int cutter_comp_side;                         // current cutter compensation side
    struct
    {
		double cc;                              // cc-value (normal) for canned cycles
		double i;                               // i-value for canned cycles
		double j;                               // j-value for canned cycles
		double k;                               // k-value for canned cycles
		int l;                                  // l-value for canned cycles
		double p;                               // p-value (dwell) for canned cycles
		double q;                               // q-value for canned cycles
		double r;                               // r-value for canned cycles
    } cycle;
    DISTANCE_MODE distance_mode;                  // absolute or incremental
    int feed_mode;                                // G_93 (inverse time) or G_94 units/min
    ON_OFF feed_override;                         // whether feed override is enabled
    double feed_rate;                             // feed rate in current units/min
    ON_OFF flood;                                 // whether flood coolant is on
    int length_offset_index;                      // for use with tool length offsets
    Units length_units;                     // millimeters or inches
    int line_length;                              // length of line last read
    char linetext[RS274NGC_TEXT_SIZE];            // text of most recent line read
    ON_OFF mist;                                  // whether mist coolant is on
    int motion_mode;                              // active G-code for motion
    int origin_index;                             // active origin (1=G54 to 9=G59.3)
    double parameters [RS274NGC_MAX_PARAMETERS];                // system parameters
    Plane plane;                            // active plane, XY-, YZ-, or XZ-plane
    ON_OFF probe_flag;                            // flag indicating probing done
    double program_x;                             // program x, used when cutter comp on
    double program_y;                             // program y, used when cutter comp on
    RETRACT_MODE retract_mode;                    // for cycles, old_z or r_plane
    int selected_tool_slot;                       // tool slot selected but not active
    int sequence_number;                          // sequence number of line last read
    double speed;                                 // current spindle speed in rpm
    SpeedFeedMode speed_feed_mode;        // independent or synched
    ON_OFF speed_override;                        // whether speed override is enabled
    Direction spindle_turning;              // direction spindle is turning
    double tool_length_offset;                    // current tool length offset
    int tool_max;                                 // highest number tool slot in carousel
    Tool tool_table [CANON_TOOL_MAX + 1];                     // index is slot number
    int tool_table_index;                         // tool index used with cutter comp
    double traverse_rate;                         // rate for traverse motions
};

   /*************************************************************************/
   /*

   Interface functions to call to tell the interpreter what to do.
   Return values indicate status of execution.
   These functions may change the state of the interpreter.

   */

struct error : std::exception
{
	int code;
	error(int code);
	virtual const char* what() const noexcept;
	virtual ~error() noexcept;
};

class rs274ngc
{
public:
private:
    setup_t _setup;

	// pointer to function that reads
	typedef void (rs274ngc::*read_function_pointer) (const char *, int *, block_t&, double *) const;
	read_function_pointer _readers[127];

	static void arc_data_comp_ijk(int move, int side, double tool_radius, double current_x, double current_y, double end_x, double end_y, double i_number, double j_number, double * center_x, double * center_y, int * turn, double tolerance);
	static void arc_data_comp_r(int move, int side, double tool_radius, double current_x, double current_y, double end_x, double end_y, double big_radius, double * center_x, double * center_y, int * turn);
	static void arc_data_ijk(int move, double current_x, double current_y, double end_x, double end_y, double i_number, double j_number, double * center_x, double * center_y, int * turn, double tolerance);
	static void arc_data_r(int move, double current_x, double current_y, double end_x, double end_y, double radius, double * center_x, double * center_y, int * turn);
	void check_g_codes(block_t& block, setup_t& settings);
	void check_items(block_t& block, setup_t& settings);
	void check_m_codes(block_t& block);
	void check_other_codes(block_t& block);
	static void close_and_downcase(char * line);
	void convert_arc(int move, block_t& block, setup_t& settings);
	void convert_arc2(int move, block_t& block, setup_t& settings, double * current1, double * current2, double * current3, double end1, double end2, double end3, double AA_end, double BB_end, double CC_end, double offset1, double offset2);
	void convert_arc_comp1(int move, block_t& block, setup_t& settings, double end_x, double end_y, double end_z, double AA_end, double BB_end, double CC_end);
	void convert_arc_comp2(int move, block_t& block, setup_t& settings, double end_x, double end_y, double end_z, double AA_end, double BB_end, double CC_end);
	void convert_axis_offsets(int g_code, block_t& block, setup_t& settings);
	void convert_comment(char * comment);
	void convert_control_mode(int g_code, setup_t& settings);
	void convert_coordinate_system(int g_code, setup_t& settings);
	void convert_cutter_compensation(int g_code, block_t& block, setup_t& settings);
	void convert_cutter_compensation_off(setup_t& settings);
	void convert_cutter_compensation_on(int side, block_t& block, setup_t& settings);
	void convert_cycle(int motion, block_t& block, setup_t& settings);
	void convert_cycle_g81(Plane plane, double x, double y, double clear_z, double bottom_z);
	void convert_cycle_g82(Plane plane, double x, double y, double clear_z, double bottom_z, double dwell);
	void convert_cycle_g83(Plane plane, double x, double y, double r, double clear_z, double bottom_z, double delta);
	void convert_cycle_g84(Plane plane, double x, double y, double clear_z, double bottom_z, Direction direction, SpeedFeedMode mode);
	void convert_cycle_g85(Plane plane, double x, double y, double clear_z, double bottom_z);
	void convert_cycle_g86(Plane plane, double x, double y, double clear_z, double bottom_z, double dwell, Direction direction);
	void convert_cycle_g87(Plane plane, double x, double offset_x, double y, double offset_y, double r, double clear_z, double middle_z, double bottom_z, Direction direction);
	void convert_cycle_g88(Plane plane, double x, double y, double bottom_z, double dwell, Direction direction);
	void convert_cycle_g89(Plane plane, double x, double y, double clear_z, double bottom_z, double dwell);
	void convert_cycle_xy(int motion, block_t& block, setup_t& settings);
	void convert_cycle_yz(int motion, block_t& block, setup_t& settings);
	void convert_cycle_zx(int motion, block_t& block, setup_t& settings);
	void convert_distance_mode(int g_code, setup_t& settings);
	void convert_dwell(double time);
	void convert_feed_mode(int g_code, setup_t& settings);
	void convert_feed_rate(block_t& block, setup_t& settings);
	void convert_g(block_t& block, setup_t& settings);
	void convert_home(int move, block_t& block, setup_t& settings);
	void convert_length_units(int g_code, setup_t& settings);
	void convert_m(block_t& block, setup_t& settings);
	void convert_modal_0(int code, block_t& block, setup_t& settings);
	void convert_motion(int motion, block_t& block, setup_t& settings);
	void convert_probe(block_t& block, setup_t& settings);
	void convert_retract_mode(int g_code, setup_t& settings);
	void convert_setup(block_t& block, setup_t& settings);
	void convert_set_plane(int g_code, setup_t& settings);
	void convert_speed(block_t& block, setup_t& settings);
	int convert_stop(block_t& block, setup_t& settings);
	void convert_straight(int move, block_t& block, setup_t& settings);
	void convert_straight_comp1(int move, block_t& block, setup_t& settings, double px, double py, double end_z, double AA_end, double BB_end, double CC_end);
	void convert_straight_comp2(int move, block_t& block, setup_t& settings, double px, double py, double end_z, double AA_end, double BB_end, double CC_end);
	void convert_tool_change(setup_t& settings);
	void convert_tool_length_offset(int g_code, block_t& block, setup_t& settings);
	void convert_tool_select(block_t& block, setup_t& settings);
	void cycle_feed(Plane plane, double end1, double end2, double end3);
	void cycle_traverse(Plane plane, double end1, double end2, double end3);
	void enhance_block(block_t& block, setup_t& settings);
	static void execute_binary(double * left, int operation, double * right);
	static void execute_binary1(double * left, int operation, double * right);
	static void execute_binary2(double * left, int operation, double * right);
	int execute_block(block_t& block, setup_t& settings);
	static void execute_unary(double * double_ptr, int operation);
	static double find_arc_length(double x1, double y1, double z1, double center_x, double center_y, int turn, double x2, double y2, double z2);
	static void find_ends(block_t& block, setup_t& settings, double * px, double * py, double * pz, double * AA_p, double * BB_p, double * CC_p);
	static void find_relative(double x1, double y1, double z1, double AA_1, double BB_1, double CC_1, double * x2, double * y2, double * z2, double * AA_2, double * BB_2, double * CC_2,setup_t& settings);
	static double find_straight_length(double x2, double y2, double z2, double AA_2, double BB_2, double CC_2, double x1, double y1, double z1, double AA_1, double BB_1, double CC_1);
	static double find_turn(double x1, double y1, double center_x, double center_y, int turn, double x2, double y2);
	void inverse_time_rate_arc(double x1, double y1, double z1, double cx, double cy, int turn, double x2, double y2, double z2, block_t& block, setup_t& settings);
	void inverse_time_rate_arc2(double start_x, double start_y, int turn1, double mid_x, double mid_y, double cx, double cy, int turn2, double end_x, double end_y, double end_z, block_t& block, setup_t& settings);
	void inverse_time_rate_as(double start_x, double start_y, int turn, double mid_x, double mid_y, double end_x, double end_y, double end_z, double AA_end, double BB_end, double CC_end, block_t& block, setup_t& settings);
	void inverse_time_rate_straight(double end_x, double end_y, double end_z, double AA_end, double BB_end, double CC_end, block_t& block, setup_t& settings);
	void parse_line(const char * line, block_t& block,setup_t& settings);
	static int precedence(int an_operator);
	void read_a(const char * line, int * counter, block_t& block, double * parameters) const;
	void read_atan(const char * line, int * counter, double * double_ptr, double * parameters) const;
	void read_b(const char * line, int * counter, block_t& block, double * parameters) const;
	void read_c(const char * line, int * counter, block_t& block, double * parameters) const;
	void read_comment(const char * line, int * counter, block_t& block, double * parameters) const;
	void read_d(const char * line, int * counter, block_t& block, double * parameters) const;
	void read_f(const char * line, int * counter, block_t& block, double * parameters) const;
	void read_g(const char * line, int * counter, block_t& block, double * parameters) const;
	void read_h(const char * line, int * counter, block_t& block, double * parameters) const;
	void read_i(const char * line, int * counter, block_t& block, double * parameters) const;
	void read_integer_unsigned(const char * line, int * counter, int * integer_ptr) const;
	void read_integer_value(const char * line, int * counter, int * integer_ptr, double * parameters) const;
	void read_items(block_t& block, const char * line, double * parameters) const;
	void read_j(const char * line, int * counter, block_t& block, double * parameters) const;
	void read_k(const char * line, int * counter, block_t& block, double * parameters) const;
	void read_l(const char * line, int * counter, block_t& block, double * parameters) const;
	void read_line_number(const char * line, int * counter, block_t& block) const;
	void read_m(const char * line, int * counter, block_t& block, double * parameters) const;
	void read_one_item(const char * line, int * counter, block_t& block, double * parameters) const;
	void read_operation(const char * line, int * counter, int * operation) const;
	void read_operation_unary(const char * line, int * counter, int * operation) const;
	void read_p(const char * line, int * counter, block_t& block, double * parameters) const;
	void read_parameter(const char * line, int * counter, double * double_ptr, double * parameters) const;
	void read_parameter_setting(const char * line, int * counter, block_t& block, double * parameters) const;
	void read_q(const char * line, int * counter, block_t& block, double * parameters) const;
	void read_r(const char * line, int * counter, block_t& block, double * parameters) const;
	void read_real_expression(const char * line, int * counter, double * hold2, double * parameters) const;
	void read_real_number(const char * line, int * counter, double * double_ptr) const;
	void read_real_value(const char * line, int * counter, double * double_ptr, double * parameters) const;
	void read_s(const char * line, int * counter, block_t& block, double * parameters) const;
	void read_t(const char * line, int * counter, block_t& block, double * parameters) const;
	int read_text(const char * command, char * raw_line, char * line, int * length);
	void read_unary(const char * line, int * counter, double * double_ptr, double * parameters) const;
	void read_x(const char * line, int * counter, block_t& block, double * parameters) const;
	void read_y(const char * line, int * counter, block_t& block, double * parameters) const;
	void read_z(const char * line, int * counter, block_t& block, double * parameters) const;
	void set_probe_data(setup_t& settings);
	void write_g_codes(const block_t* block, setup_t& settings);
	void write_m_codes(const block_t* block, setup_t& settings);
	void write_settings(setup_t& settings);

private:
	virtual void interp_init() =0;

	virtual void offset_origin(const Position& pos) =0;
	
	virtual void units(Units u) =0;
	virtual Units units() const =0;
	
	virtual void plane(Plane pl) =0;
	virtual Plane plane() const =0;
	
	virtual void rapid_rate(double rate) =0;
	virtual double rapid_rate() const =0;
	
	virtual void feed_rate(double rate) =0;
	virtual double feed_rate() const =0;
	virtual void feed_reference(FeedReference reference) =0;
	
	virtual void motion_mode(Motion mode) =0;
	virtual Motion motion_mode() const =0;
	
	virtual void cutter_radius_comp(double radius) =0;
	virtual void cutter_radius_comp_start(Side direction) =0;
	virtual void cutter_radius_comp_stop() =0;
	
	virtual void speed_feed_sync_start() =0;
	virtual void speed_feed_sync_stop() =0;
	
	virtual void rapid(const Position& pos) =0;
	virtual void arc(double end0, double end1, double axis0, double axis1, int rotation, double end_point, double a, double b, double c) =0;
	virtual void linear(const Position& pos) =0;
	virtual void probe(const Position& pos) =0;
	virtual void dwell(double seconds) =0;

	virtual void spindle_start_clockwise() =0;
	virtual void spindle_start_counterclockwise() =0;
	virtual void spindle_stop() =0;
	virtual Direction spindle_state() const =0;
	virtual void spindle_speed(double r) =0;
	virtual double spindle_speed() const =0;
	virtual void spindle_orient(double orientation, Direction direction) =0;

	virtual void tool_length_offset(double length) =0;
	virtual void tool_change(int slot) =0;
	virtual void tool_select(int i) =0;
	virtual int tool_slot() const =0;
	virtual Tool tool(int pocket) const =0;
	virtual int tool_max() const =0;

	virtual void axis_clamp(Axis axis) =0;
	virtual void axis_unclamp(Axis axis) =0;

	virtual void comment(const char *s) =0;

	virtual void feed_override_disable() =0;
	virtual void feed_override_enable() =0;

	virtual void speed_override_disable() =0;
	virtual void speed_override_enable() =0;

	virtual void coolant_flood_off() =0;
	virtual void coolant_flood_on() =0;
	virtual bool coolant_flood() const =0;
	
	virtual void coolant_mist_off() =0;
	virtual void coolant_mist_on() =0;
	virtual bool coolant_mist() const =0;

	virtual void message(const char *s) =0;

	virtual void pallet_shuttle() =0;

	virtual void probe_off() =0;
	virtual void probe_on() =0;
	virtual Position probe_position() const =0;
	virtual double probe_value() const =0;

	virtual void program_optional_stop() =0;
	virtual void program_end() =0;
	virtual void program_stop() =0;

	virtual void get_parameter_filename(char* filename, size_t max_size) const =0;
	virtual Position current_position() const =0;
	virtual bool queue_empty() const =0;

public:

	rs274ngc();

	   // execute a line of NC code
	int execute();

	   // stop running
	void exit();

	   // get ready to run
	void init();

	   // load a tool table
	void load_tool_table();

	   // read the command
	int read(const char * command);

	   // reset yourself
	void reset();

	   // restore interpreter variables from a file
	void restore_parameters(const char * filename);

	   // save interpreter variables to file
	void save_parameters(const char * filename, const double parameters[]);

	   // synchronize your internal model with the external world
	void synch();

	   /*************************************************************************/
	   /* 

	   Interface functions to call to get information from the interpreter.
	   If a function has a return value, the return value contains the information.
	   If a function returns nothing, information is copied into one of the
	   arguments to the function. These functions do not change the state of
	   the interpreter.

	   */

	   // copy active G codes into array [0]..[11]
	void active_g_codes(int * codes);

	   // copy active M codes into array [0]..[6]
	void active_m_codes(int * codes);

	   // copy active F, S settings into array [0]..[2]
	void active_settings(double * settings);

	   // copy the text of the error message whose number is error_code into the
	   // error_text array, but stop at max_size if the text is longer.
	void error_text(int error_code, char * error_text, size_t max_size);

	   // return the length of the most recently read line
	int line_length();

	   // copy the text of the most recently read line into the line_text array,
	   // but stop at max_size if the text is longer
	void line_text(char * line_text, size_t max_size);

	   // return the current sequence number (how many lines read)
	int sequence_number();

};

#endif
