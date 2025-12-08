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

#define INITIAL_PROBLEM_LIST_LENGTH 4

enum {
	OPERATOR_MULTIPLY,
	OPERATOR_ADD,
	OPERATOR_COUNT
};

typedef struct {
	u32 *parameters;
	u32 parameter_count;
	i32 operator;
} problem;

typedef struct {
	problem *problems;
	u32 problem_count;
	u32 problem_index;
	u32 parameter_index;
	b32 parsing;
} parse_data;

typedef struct {
	b32 last_char_was_op;
	b32 doing_preparse;
	u32 current_problem_width;
	u32 problem_list_length;
} preparse_parse_data;

static parse_data parse;
static preparse_parse_data preparse;

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
	preparse.last_char_was_op = false;
	preparse.doing_preparse = true;
	preparse.problem_list_length = INITIAL_PROBLEM_LIST_LENGTH;
	parse.problems = (problem*)malloc(INITIAL_PROBLEM_LIST_LENGTH * sizeof(problem));
	u64 input_index = input_size - 1;
	while(preparse.doing_preparse)
	{
		switch(input[input_index])
		{
			case '\n':
			{
				/* do nothing */
				preparse.last_char_was_op = false;
			} break;
			case '*':
			{
				preparse.current_problem_width++;
				parse.problems[parse.problem_count].parameter_count = preparse.current_problem_width;
				parse.problems[parse.problem_count].parameters = (u32*)malloc(sizeof(u32) * preparse.current_problem_width);
				/* zero initialize all parameters */
				u32 parameter_index = 0;
				for( ; parameter_index < preparse.current_problem_width; parameter_index++)
				{
					parse.problems[parse.problem_count].parameters[parameter_index] = 0;
				}
				parse.problems[parse.problem_count].operator = OPERATOR_MULTIPLY;
				parse.problem_count++;
				if(parse.problem_count >= preparse.problem_list_length)
				{
					preparse.problem_list_length *= 2;
					parse.problems = (problem*)realloc(parse.problems, sizeof(problem) * preparse.problem_list_length);
				}
				preparse.current_problem_width = 0;
				preparse.last_char_was_op = true;
			} break;
			case '+':
			{
				preparse.current_problem_width++;
				parse.problems[parse.problem_count].parameter_count = preparse.current_problem_width;
				parse.problems[parse.problem_count].parameters = (u32*)malloc(sizeof(u32) * preparse.current_problem_width);
				/* zero initialize all parameters */
				u32 parameter_index = 0;
				for( ; parameter_index < preparse.current_problem_width; parameter_index++)
				{
					parse.problems[parse.problem_count].parameters[parameter_index] = 0;
				}
				parse.problems[parse.problem_count].operator = OPERATOR_ADD;
				parse.problem_count++;
				if(parse.problem_count >= preparse.problem_list_length)
				{
					preparse.problem_list_length *= 2;
					parse.problems = (problem*)realloc(parse.problems, sizeof(problem) * preparse.problem_list_length);
				}
				preparse.current_problem_width = 0;
				preparse.last_char_was_op = true;
			} break;
			case ' ':
			{
				if(!preparse.last_char_was_op)
				{
					preparse.current_problem_width++;
				}
				preparse.last_char_was_op = false;
			} break;
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
				preparse.doing_preparse = false;
			} break;
			default:
			{
				_assert_log(0, "unexpected char during preparse: %c (%x)", input[input_index], input[input_index]);
			} break;
		}
		input_index--;
	}
	PROFILER_FINISH_TIMING_BLOCK(preparse);

	PROFILER_START_TIMING_BANDWIDTH(parse, input_size);
	input_index = 0;
	parse.parsing = true;
	parse.problem_index = parse.problem_count - 1;
	while(input_index < input_size && parse.parsing)
	{
		switch(input[input_index])
		{
			case '\n':
			{
				parse.problem_index = parse.problem_count - 1;
				parse.parameter_index = 0;
			} break;
			case '*':
			{
				/* stop parsing, bc we already preparsed the ops */
				parse.parsing = false;
			} break;
			case '+':
			{
				/* stop parsing, bc we already preparsed the ops */
				parse.parsing = false;
			} break;
			case ' ':
			{
				u32 current_problem_parameter_count = parse.problems[parse.problem_index].parameter_count;
				if(parse.parameter_index < current_problem_parameter_count)
				{
					parse.parameter_index++;
				}
				else
				{
					parse.problem_index--;
					_assert(parse.problem_count >= 0);
					parse.parameter_index = 0;
				}
			} break;
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
				u32 current_problem_parameter_count = parse.problems[parse.problem_index].parameter_count;
				_assert_log(parse.parameter_index < current_problem_parameter_count, 
					"PARSING char %u (%c, 0x%x): param count = %u, current param = %u (problem %u)", 
					input_index, input[input_index], input[input_index],
					current_problem_parameter_count, parse.parameter_index, parse.problem_index);

				parse.problems[parse.problem_index].parameters[parse.parameter_index] = 
					parse.problems[parse.problem_index].parameters[parse.parameter_index] * 10 + (input[input_index] - 48);

				parse.parameter_index++;
			} break;
			default:
			{
				_assert_log(0, "unexpected char during parse: %c (0x%x)", input[input_index], input[input_index]);
			} break;
		}
		input_index++;
	}
	PROFILER_FINISH_TIMING_BLOCK(parse);

	u64 grand_total = 0;
	PROFILER_START_TIMING_BLOCK(grand_total_calculation);
	u32 problem_index = 0;
	for( ; problem_index < parse.problem_count; problem_index++)
	{
		u64 problem_total = 0;
		u32 parameter_index = 0;
		problem *p = &(parse.problems[problem_index]);
		for( ; parameter_index < p->parameter_count; parameter_index++)
		{
			if(p->operator == OPERATOR_MULTIPLY)
			{
				if(parameter_index == 0)
				{
					problem_total += p->parameters[parameter_index];
				}
				else
				{
					problem_total *= p->parameters[parameter_index];
				}
			}
			else if(p->operator == OPERATOR_ADD)
			{
				problem_total += p->parameters[parameter_index];
			}
			else
			{
				_assert_log(0, "unexpected operator encountered during grand total calculation: %d", p->operator);
			}
		}
		grand_total += problem_total;
	}
	PROFILER_FINISH_TIMING_BLOCK(grand_total_calculation);

	log_info("GRAND TOTAL: %llu", grand_total);

	finish_and_print_profile(log_trace);

	/* NOTE(josh): I am doing this purely to avoid setting off ASAN */
	problem_index = 0;
	for( ; problem_index < parse.problem_count; problem_index++)
	{
		free(parse.problems[problem_index].parameters);
	}
	free(parse.problems);

	return(0);
}
