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

#define INITIAL_PARAMETER_LIST_LENGTH 4

enum {
	PARSE_STAGE_PARAMETERS,
	PARSE_STAGE_OPERATORS,
	PARSE_STAGE_COUNT
};

enum {
	OP_MULT,
	OP_ADD,
	OP_COUNT
};

typedef struct {
	/* XXX: */
	u32 parameter_list_length;
	u32 parameter_index; /* row basically */
	u32 problem_index; /* column basically */
	b32 parsing_whitespace;
	b32 last_char_was_op;
	i32 parse_stage;
} parse_data;

static parse_data parse;

int main(int argc, char **argv)
{
	if(argc != 2)
	{
		log_error("USAGE -> ./prog [input_file]");
		return(-1);
	}

	u64 input_size = get_file_size(argv[1]);
	char *input = (char*)read_file_mmapped(argv[1]);

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

	/* preparse to determine problem count */
	u32 problem_count = 0;
	u64 input_index = 0;
	b32 parsing = true;
	while(parsing)
	{
		switch(input[input_index])
		{
			case ' ':
			{
				if(!parse.parsing_whitespace)
				{
					parse.parsing_whitespace = true;
					problem_count++;
				}
			} break;
			case '\n':
			{
				if(!parse.parsing_whitespace)
				{
					parse.parsing_whitespace = true;
					problem_count++;
				}
				parsing = false;
			} break;
			case '0':
			{
				parse.parsing_whitespace = false;
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
				parse.parsing_whitespace = false;
			} break;
			default:
			{
				_assert_log(0, "unexpected char during parse: %c (%x)", input[input_index], input[input_index]);
			} break;
		}
		input_index++;
	}
	
	i32 **problem_parameter_lists = (i32**)malloc(problem_count * sizeof(i32*));
	i32 *problem_operator_list = (i32*)malloc(problem_count * sizeof(i32));

	u32 problem_index = 0;
	for( ; problem_index < problem_count; problem_index++)
	{
		problem_parameter_lists[problem_index] = (i32*)malloc(sizeof(i32) * INITIAL_PARAMETER_LIST_LENGTH);
	}
	parse.parameter_list_length = INITIAL_PARAMETER_LIST_LENGTH;

	/* ensure first parameter is set to 0 */
	/* XXX: we should probably just set stuff to 0 up front, but the realloc's make that not simple */
	problem_parameter_lists[0][0] = 0;
	input_index = 0;
	parse.parsing_whitespace = false;
	while(input_index < input_size)
	{
		switch(input[input_index])
		{
			case '\n':
			{
				switch(parse.parse_stage)
				{
					case PARSE_STAGE_PARAMETERS:
					{
						parse.parameter_index++;
						if(parse.parameter_index >= parse.parameter_list_length)
						{
							problem_index = 0;
							for( ; problem_index < problem_count; problem_index++)
							{
								problem_parameter_lists[problem_index] = 
									(i32*)realloc(problem_parameter_lists[problem_index], 
												 sizeof(i32) * parse.parameter_list_length * 2);
							}
							parse.parameter_list_length *= 2;
						}
					} break;
					case PARSE_STAGE_OPERATORS:
					{
						/* do nothing */
					} break;
					default:
					{
						_assert_log(0, "parse encountered unexpected parse stage: %d", parse.parse_stage);
					}
				}
				parse.parsing_whitespace = true;
				parse.last_char_was_op = false;
				parse.problem_index = 0;
				problem_parameter_lists[0][parse.parameter_index] = 0;
			} break;
			case '*':
			{
				_assert(!parse.last_char_was_op);
				if(parse.parse_stage == PARSE_STAGE_PARAMETERS)
				{
					parse.parse_stage = PARSE_STAGE_OPERATORS;
				}
				problem_operator_list[parse.problem_index] = OP_MULT;
				parse.parsing_whitespace = false;
				parse.last_char_was_op = true;
			} break;
			case '+':
			{
				_assert(!parse.last_char_was_op);
				if(parse.parse_stage == PARSE_STAGE_PARAMETERS)
				{
					parse.parse_stage = PARSE_STAGE_OPERATORS;
				}
				problem_operator_list[parse.problem_index] = OP_ADD;
				parse.parsing_whitespace = false;
				parse.last_char_was_op = true;
			} break;
			case ' ':
			{
				if(!parse.parsing_whitespace)
				{
					parse.parsing_whitespace = true;
					if(parse.problem_index < problem_count - 1)
					{
						parse.problem_index++;
						if(parse.parse_stage == PARSE_STAGE_PARAMETERS)
						{
							/* ensure parameters are set to 0 */
							i32 *param_list = problem_parameter_lists[parse.problem_index];
							param_list[parse.parameter_index] = 0;
						}
					}
				}
				parse.last_char_was_op = false;
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
				_assert(parse.parse_stage == PARSE_STAGE_PARAMETERS);
				i32 *param_list = problem_parameter_lists[parse.problem_index];
				param_list[parse.parameter_index] = param_list[parse.parameter_index] * 10 + (input[input_index] - 48);
				parse.parsing_whitespace = false;
				parse.last_char_was_op = false;
			} break;
			default:
			{
				_assert_log(0, "unexpected char during parse: %c (%x)", input[input_index], input[input_index]);
			} break;
		}
		input_index++;
	}

	u64 grand_total = 0;
	u64 problem_total = 0;
	
	problem_index = 0;
	for( ; problem_index < problem_count; problem_index++)
	{
		i32 *param_list = problem_parameter_lists[problem_index];
		u32 parameter_index = 0;
		for( ; parameter_index < parse.parameter_index; parameter_index++)
		{
			if(problem_operator_list[problem_index] == OP_MULT)
			{
				if(parameter_index == 0)
				{
					problem_total += param_list[parameter_index];
				}
				else
				{
					problem_total *= param_list[parameter_index];
				}
	  		}
			else if(problem_operator_list[problem_index] == OP_ADD)
			{
				problem_total += param_list[parameter_index];
	  		}
			else
			{
				_assert_log(0, "unexpected operator: %d", problem_operator_list[problem_index]);
			}
		}
		grand_total += problem_total;
		problem_total = 0;
	}

	log_info("GRAND TOTAL: %llu", grand_total);

	problem_index = 0;
	for( ; problem_index < problem_count; problem_index++)
	{
		free(problem_parameter_lists[problem_index]);
	}

	free(problem_parameter_lists);
	free(problem_operator_list);

	return(0);
}
