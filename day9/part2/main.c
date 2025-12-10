typedef double f64;
typedef float f32;

typedef signed char i8;
typedef unsigned char u8;
typedef short i16;
typedef unsigned short u16;
typedef int i32;
typedef unsigned int u32;
typedef long long i64;
typedef unsigned long long u64;

typedef unsigned int b32;

#define true 1
#define false 0

#include "../../include/linux_util.c"

#define PROFILER 1
#include "../../include/profiler.c"

typedef struct {
	u64 x;
	u64 y;
} point;

typedef struct {
	point current_point;
	u64 max_x;
	u64 min_x;
	u64 max_y;
	u64 min_y;
	b32 parsing_x;
} parse_data;

typedef struct {
	u32 point_index_top_left_closest;
	u32 point_index_top_right_closest;
	u32 point_index_bot_left_closest;
	u32 point_index_bot_right_closest;
	u64 shortest_distance_top_left;
	u64 shortest_distance_top_right;
	u64 shortest_distance_bot_left;
	u64 shortest_distance_bot_right;
} largest_area_calculation_data;

static parse_data parse;

static u64 distance_squared(u64 x1, u64 y1, u64 x2, u64 y2)
{
	return( (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1) );
}

int main(int argc, char **argv)
{
	start_profile();

	if(argc != 2) { log_error("USAGE -> ./prog [input_file]"); return(-1); }

	u64 input_size = get_file_size(argv[1]);
	PROFILER_START_TIMING_BANDWIDTH(file_read, input_size);
	char *input = (char*)read_file_mmapped(argv[1]);
	PROFILER_FINISH_TIMING_BLOCK(file_read);

	if(input_size == 0) { log_error("input file '%s' has size: 0. terminating.", argv[1]); return(-1); }
	if(!input) { log_error("could not read input from '%s'. terminating.", argv[1]); return(-1); }

	log_trace("input:\n%s", input);
	log_trace("input size: %llu", input_size);

	/* TODO: get all the points, and keep track of min x and min y, as well as max x and max y, bc it will be points
	 * with those that give the biggest rectangle, or like I guess try and find the 4 farthest corners? ? */
	/* if you have max/min x/y, then you just find the points closest to that, yeah ? */
	PROFILER_START_TIMING_BANDWIDTH(parse, input_size);
	point *point_array = (point *)dynamic_array_create(sizeof(point), 2);
	u64 input_index = 0;
	parse.parsing_x = true;
	while(input_index < input_size)
	{
		switch(input[input_index])
		{
			case '\n':
			{
				if(parse.current_point.x > parse.max_x) { parse.max_x = parse.current_point.x; }
				if(parse.current_point.x < parse.min_x) { parse.min_x = parse.current_point.x; }
				if(parse.current_point.y > parse.max_y) { parse.max_y = parse.current_point.y; }
				if(parse.current_point.y < parse.min_y) { parse.min_y = parse.current_point.y; }
				if(parse.min_x == 0) { parse.min_x = parse.current_point.x; }
				if(parse.min_y == 0) { parse.min_y = parse.current_point.y; }

				point_array = (point *)dynamic_array_add(point_array, &(parse.current_point));

				parse.current_point.x = 0;
				parse.current_point.y = 0;
				parse.parsing_x = true;
			} break;
			case ',':
			{
				parse.parsing_x = false;
			} break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			{
				if(parse.parsing_x)
				{
					parse.current_point.x = parse.current_point.x * 10 + (input[input_index] - 48);
				}
				else
				{
					parse.current_point.y = parse.current_point.y * 10 + (input[input_index] - 48);
				}
			} break;
			default:
			{
				_assert_log(0, "unexpected char during parse: %c (0x%x)", input[input_index], input[input_index]);
			} break;
		}
		input_index++;
	}
	PROFILER_FINISH_TIMING_BLOCK(parse);

	log_debug("\nmax x: %u\nmax y: %u\nmin x: %u\nmin y: %u", parse.max_x, parse.max_y, parse.min_x, parse.min_y);

	PROFILER_START_TIMING_BANDWIDTH(finding_largest_area, sizeof(point) * dynamic_array_length(point_array));
	largest_area_calculation_data calc_data;
	zero_memory(&calc_data, sizeof(calc_data));
	u32 point_index = 0;
	for( ; point_index < dynamic_array_length(point_array); point_index++)
	{
		u64 dist_top_left  = distance_squared(parse.min_x, parse.min_y, point_array[point_index].x, point_array[point_index].y);
		u64 dist_top_right = distance_squared(parse.max_x, parse.min_y, point_array[point_index].x, point_array[point_index].y);
		u64 dist_bot_left  = distance_squared(parse.min_x, parse.max_y, point_array[point_index].x, point_array[point_index].y);
		u64 dist_bot_right = distance_squared(parse.max_x, parse.max_y, point_array[point_index].x, point_array[point_index].y);
		if(point_index == 0)
		{
			calc_data.shortest_distance_top_left = dist_top_left;
			calc_data.shortest_distance_top_right = dist_top_right;
			calc_data.shortest_distance_bot_left = dist_bot_left;
			calc_data.shortest_distance_bot_right = dist_bot_right;
			continue;
		}
		if(dist_top_left < calc_data.shortest_distance_top_left) 
		{
			calc_data.point_index_top_left_closest = point_index;
			calc_data.shortest_distance_top_left = dist_top_left;
		}
		if(dist_top_right < calc_data.shortest_distance_top_right) 
		{
			calc_data.point_index_top_right_closest = point_index;
			calc_data.shortest_distance_top_right = dist_top_right;
		}
		if(dist_bot_left < calc_data.shortest_distance_bot_left) 
		{
			calc_data.point_index_bot_left_closest = point_index;
			calc_data.shortest_distance_bot_left = dist_bot_left;
		}
		if(dist_bot_right < calc_data.shortest_distance_bot_right) 
		{
			calc_data.point_index_bot_right_closest = point_index;
			calc_data.shortest_distance_bot_right = dist_bot_right;
		}
	}

	log_debug("farthest top left point (index %u): %u, %u", 
		   calc_data.point_index_top_left_closest, 
		   point_array[calc_data.point_index_top_left_closest].x, 
		   point_array[calc_data.point_index_top_left_closest].y); 
	log_debug("farthest top right point (index %u): %u, %u", 
		   calc_data.point_index_top_right_closest, 
		   point_array[calc_data.point_index_top_right_closest].x, 
		   point_array[calc_data.point_index_top_right_closest].y); 
	log_debug("farthest bot left point (index %u): %u, %u", 
		   calc_data.point_index_bot_left_closest, 
		   point_array[calc_data.point_index_bot_left_closest].x, 
		   point_array[calc_data.point_index_bot_left_closest].y); 
	log_debug("farthest bot right point (index %u): %u, %u", 
		   calc_data.point_index_bot_right_closest, 
		   point_array[calc_data.point_index_bot_right_closest].x, 
		   point_array[calc_data.point_index_bot_right_closest].y); 

	u64 top_left_to_bot_right_area = 
		(point_array[calc_data.point_index_bot_right_closest].x - point_array[calc_data.point_index_top_left_closest].x + 1) *
		(point_array[calc_data.point_index_bot_right_closest].y - point_array[calc_data.point_index_top_left_closest].y + 1); 
	u64 bot_left_to_top_right_area = 
		(point_array[calc_data.point_index_top_right_closest].x - point_array[calc_data.point_index_bot_left_closest].x + 1) *
		(point_array[calc_data.point_index_bot_left_closest].y - point_array[calc_data.point_index_top_right_closest].y + 1); 

	log_debug("top left -> bot right area: %llu", top_left_to_bot_right_area);
	log_debug("bot left -> top right area: %llu", bot_left_to_top_right_area);

	u64 largest_area = 0;
	if(top_left_to_bot_right_area > bot_left_to_top_right_area)
	{
		largest_area = top_left_to_bot_right_area;
	}
	else
	{
		largest_area = bot_left_to_top_right_area;
	}
	PROFILER_FINISH_TIMING_BLOCK(finding_largest_area);

	finish_and_print_profile(log_trace);

	log_info("largest area: %llu", largest_area);

	dynamic_array_destroy(point_array);

	return(0);
}
