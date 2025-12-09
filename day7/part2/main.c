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
	u64 *manifold_column_beam_count_list;
	u32 manifold_width;
} parse_data;

static parse_data parse;

int main(int argc, char **argv)
{
	start_profile();

	if(argc != 2)
	{
		log_error("USAGE -> ./prog [input_file]");
		return(-1);
	}

	u64 input_size = get_file_size(argv[1]);
	PROFILER_START_TIMING_BANDWIDTH(file_read, input_size);
	char *input = (char*)read_file_mmapped(argv[1]);
	PROFILER_FINISH_TIMING_BLOCK(file_read);

	if(input_size == 0)
	{
		log_error("input file '%s' has size: 0. terminating.", argv[1]);
		return(-1);
	}

	if(!input)
	{
		log_error("could not read input from '%s'. terminating.", argv[1]);
		return(-1);
	}

	PROFILER_START_TIMING_BLOCK(preparse);
	u64 input_index = 0;
	b32 preparsing = true;
	i32 beam_start_index = -1;
	while(input_index < input_size && preparsing)
	{
		switch(input[input_index])
		{
			case '\n':
			{
				preparsing = false;
			} break;
			case '.':
			{
				parse.manifold_width++;
			} break;
			case 'S':
			{
				beam_start_index = input_index;
				parse.manifold_width++;
			} break;
			default:
			{
				_assert_log(0, "unexpected char during parse: %c (0x%x)", input[input_index], input[input_index]);
			} break;
		}
		input_index++;
	}
	_assert(beam_start_index > 0 && beam_start_index < parse.manifold_width);
	parse.manifold_column_beam_count_list = (u64*)malloc(sizeof(u64) * parse.manifold_width);
	_assert(zero_memory(parse.manifold_column_beam_count_list, sizeof(u64) * parse.manifold_width));
	parse.manifold_column_beam_count_list[beam_start_index] = 1;
	PROFILER_FINISH_TIMING_BLOCK(preparse);

	PROFILER_START_TIMING_BANDWIDTH(parse, input_size);
	input_index = parse.manifold_width + 1;
	while(input_index < input_size)
	{
		switch(input[input_index])
		{
			case '\n':
			{
				/* do nothing */
				u32 column = input_index % (parse.manifold_width + 1);
				_assert((column % (parse.manifold_width + 1)) == parse.manifold_width);
			} break;
			case '.':
			{
				/* do nothing */
			} break;
			case '^':
			{
				u32 column = input_index % (parse.manifold_width + 1);
				_assert((column % (parse.manifold_width + 1)) != parse.manifold_width);
				if(parse.manifold_column_beam_count_list[column] > 0)
				{
					if(column > 0)
					{
						parse.manifold_column_beam_count_list[column - 1] += parse.manifold_column_beam_count_list[column];
					}
					if(column < parse.manifold_width - 1)
					{
						parse.manifold_column_beam_count_list[column + 1] += parse.manifold_column_beam_count_list[column];
					}
				}
				parse.manifold_column_beam_count_list[column] = 0;
			} break;
			default:
			{
				_assert_log(0, "unexpected char during parse: %c (0x%x)", input[input_index], input[input_index]);
			} break;
		}
		input_index++;
	}
	PROFILER_FINISH_TIMING_BLOCK(parse);


	PROFILER_START_TIMING_BLOCK(timeline_count_calculation);
	u32 column_index = 0;
	u64 timeline_count = 0;
	for( ; column_index < parse.manifold_width; column_index++)
	{
		timeline_count += parse.manifold_column_beam_count_list[column_index];
	}
	PROFILER_FINISH_TIMING_BLOCK(timeline_count_calculation);

	finish_and_print_profile(log_trace);

	log_info("number of timelines: %llu", timeline_count);

	return(0);
}
