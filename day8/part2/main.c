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

#include <math.h> /* NOTE(josh): for sqrt() */
#include "../../include/linux_util.c"

#define PROFILER 1
#include "../../include/profiler.c"

#define INITIAL_JUNCTION_BOX_ARRAY_SIZE 16

enum {
	POSITION_COMPONENT_X,
	POSITION_COMPONENT_Y,
	POSITION_COMPONENT_Z,
	POSITION_COMPONENT_COUNT
};

typedef struct
{
	u64 x;
	u64 y;
	u64 z;
} position;

typedef struct
{
	position pos;
	i32 parent_box;
} junction_box;

typedef struct
{
	u64 straight_line_distance;
	u32 box_one_id;
	u32 box_two_id;
} junction_box_distance;

typedef struct {
	junction_box current_box;
	char *input;
	u64 input_index;
	u64 input_size;
	u32 junction_box_index;
	i32 current_position_component;
} parse_data;

static junction_box *global_junction_box_array = 0;
static junction_box_distance *global_junction_box_distance_array = 0;
static u32 *global_circuit_sizes = 0;

static void parse_input(parse_data *parse);
static u64 calculate_junction_box_distance(u32 box_one_id, u32 box_two_id);
static i32 box_get_circuit(u32 box_id);

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

	global_junction_box_array = (junction_box *)dynamic_array_create(sizeof(junction_box), INITIAL_JUNCTION_BOX_ARRAY_SIZE);
	parse_data parse;
	zero_memory(&parse, sizeof(parse_data));
	parse.input = input;
	parse.input_size = input_size;
	parse_input(&parse);

	u32 junction_box_count = parse.junction_box_index;
	/* NOTE(josh): allocating an array that's about equal to number of junction boxes squared divided by 2, since that's 
	 * roughly the possible # of combinations of boxes there could be */
	global_junction_box_distance_array = 
		(junction_box_distance *)dynamic_array_create(sizeof(junction_box_distance), 
			(junction_box_count * junction_box_count) / 2);

	PROFILER_START_TIMING_BANDWIDTH(distance_calculations, junction_box_count * sizeof(junction_box));

	u32 box_one_index = 0;
	for( ; box_one_index < junction_box_count; box_one_index++)
	{
		u32 box_two_index = box_one_index + 1;
		for( ; box_two_index < junction_box_count; box_two_index++)
		{
			junction_box_distance dist;
			dist.box_one_id = box_one_index;
			dist.box_two_id = box_two_index;
			dist.straight_line_distance = calculate_junction_box_distance(box_one_index, box_two_index);

			u32 dist_array_index = 0;
			b32 inserted = false;
			while(dist_array_index < dynamic_array_length(global_junction_box_distance_array))
			{
				if(global_junction_box_distance_array[dist_array_index].straight_line_distance >
					dist.straight_line_distance)
				{
					dynamic_array_insert(global_junction_box_distance_array, &dist, dist_array_index);
					inserted = true;
					break;
				}
				dist_array_index++;
			}
			if(!inserted)
			{
				dynamic_array_add(global_junction_box_distance_array, &dist);
			}
		}
	}

	PROFILER_FINISH_TIMING_BLOCK(distance_calculations);

	PROFILER_START_TIMING_BLOCK(circuit_construction);
	u32 circiut_counter = 0;
	u32 connections_made = 0;
	global_circuit_sizes = malloc(sizeof(u32) * junction_box_count);
	zero_memory(global_circuit_sizes, sizeof(u32) * junction_box_count);
	u32 circuit_count = junction_box_count;
	u32 final_box_one;
	u32 final_box_two;
	while(connections_made < dynamic_array_length(global_junction_box_distance_array))
	{
		junction_box_distance dist = global_junction_box_distance_array[connections_made];
		i32 box_one_circuit = box_get_circuit(dist.box_one_id);
		i32 box_two_circuit = box_get_circuit(dist.box_two_id);
		if(box_one_circuit == box_two_circuit)
		{
			/* do nothing */
		}
		else
		{
			global_junction_box_array[box_one_circuit].parent_box = box_two_circuit;
			circuit_count--;
		}
		connections_made++;
		if(circuit_count == 1)
		{
			final_box_one = dist.box_one_id;
			final_box_two = dist.box_two_id;
			break;
		}
	}
	PROFILER_FINISH_TIMING_BLOCK(circuit_construction);

	finish_and_print_profile(log_trace);

	u64 result = global_junction_box_array[final_box_one].pos.x * global_junction_box_array[final_box_two].pos.x;
	log_info("result: %llu", result);

	dynamic_array_destroy(global_junction_box_array);
	dynamic_array_destroy(global_junction_box_distance_array);

	return(0);
}

static void parse_input(parse_data *parse)
{
	PROFILER_START_TIMING_BANDWIDTH(parse, parse->input_size);
	while(parse->input_index < parse->input_size)
	{
		switch(parse->input[parse->input_index])
		{
			case '\n':
			{
				parse->current_box.parent_box = parse->junction_box_index;
				global_junction_box_array = dynamic_array_add(global_junction_box_array, &(parse->current_box));

				parse->current_box.pos.x = 0;
				parse->current_box.pos.y = 0;
				parse->current_box.pos.z = 0;

				parse->current_position_component = 0;
				parse->junction_box_index++;
			} break;
			case ',':
			{
				parse->current_position_component++;
			} break;
			/* NOTE(josh): just doesn't seem to be a special case for 0 in this problem,
			 * none of the positions in the input have a component that's just '0' */
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
				switch(parse->current_position_component)
				{
					case POSITION_COMPONENT_X:
					{
						parse->current_box.pos.x = parse->current_box.pos.x * 10 + (parse->input[parse->input_index] - 48);
					} break;
					case POSITION_COMPONENT_Y:
					{
						parse->current_box.pos.y = parse->current_box.pos.y * 10 + (parse->input[parse->input_index] - 48);
					} break;
					case POSITION_COMPONENT_Z:
					{
						parse->current_box.pos.z = parse->current_box.pos.z * 10 + (parse->input[parse->input_index] - 48);
					} break;
					default:
					{
						_assert_log(0, "unexpected position component: %d", parse->current_position_component);
					} break;
				}
			} break;
			default:
			{
				_assert_log(0, "unexpected char during parse: %c (0x%x)", 
					parse->input[parse->input_index], parse->input[parse->input_index]);
			} break;
		}
		(parse->input_index)++;
	}
	PROFILER_FINISH_TIMING_BLOCK(parse);
}

static u64 calculate_junction_box_distance(u32 box_one_id, u32 box_two_id)
{
	junction_box box_one = global_junction_box_array[box_one_id];
	junction_box box_two = global_junction_box_array[box_two_id];

	u64 result = 
		(box_one.pos.x - box_two.pos.x) * (box_one.pos.x - box_two.pos.x) + 
		(box_one.pos.y - box_two.pos.y) * (box_one.pos.y - box_two.pos.y) + 
		(box_one.pos.z - box_two.pos.z) * (box_one.pos.z - box_two.pos.z);

	return(result);
}

static i32 box_get_circuit(u32 box_id)
{

	i32 box = box_id;
	while(box != global_junction_box_array[box].parent_box)
	{
		box = global_junction_box_array[box].parent_box;
	}
	return(box);
}
