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

#include "linux_util.c"

enum {
	PARSE_STAGE_FINDING_NEWLINE,
	PARSE_STAGE_LEFT_DIGIT,
	PARSE_STAGE_RIGHT_DIGIT,
	PARSE_STAGE_COUNT
};

typedef struct {
	u64 left_digit_index;
	u64 line_start_index;
	u64 line_length;
	u64 joltage;
	i32 parse_stage;
	u8 left_digit;
	u8 right_digit;
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

	log_trace("input:\n%s", input);
	log_trace("input size: %llu", input_size);

	u64 input_index = 0;
	/* TODO: write code for part 2
	 * - now use 12 batteries from each memory bank */
	while(input_index < input_size)
	{
		switch(input[input_index])
		{
			case '\n':
			{
				switch(parse.parse_stage)
				{
					case PARSE_STAGE_LEFT_DIGIT:
					{
						parse.parse_stage = PARSE_STAGE_RIGHT_DIGIT;
						input_index = parse.line_start_index;
						continue; /* NOTE(josh): skips the input_index++; */
					} break;
					case PARSE_STAGE_RIGHT_DIGIT:
					{
						u32 joltage = parse.left_digit * 10 + parse.right_digit;
						parse.joltage += joltage;

						log_debug("joltage: %u%u", parse.left_digit, parse.right_digit);
						parse.parse_stage = PARSE_STAGE_FINDING_NEWLINE;
						parse.line_start_index = parse.line_start_index + parse.line_length + 1;
						parse.left_digit = 0;
						parse.right_digit = 0;
						parse.left_digit_index = 0;
						parse.line_length = 0;
					} break;
					case PARSE_STAGE_FINDING_NEWLINE:
					{
						parse.parse_stage = PARSE_STAGE_LEFT_DIGIT;
						input_index = parse.line_start_index;
						continue; /* NOTE(josh): skips the input_index++; */
					} break;
					default:
					{
						_assert_message(0, "unexpected value for parse stage");
					} break;
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
				switch(parse.parse_stage)
				{
					case PARSE_STAGE_LEFT_DIGIT:
					{
						if((input[input_index] - 48) > parse.left_digit && 
							input_index < (parse.line_start_index + parse.line_length - 1))
						{
							parse.left_digit = input[input_index] - 48;
							parse.left_digit_index = input_index;
						}
					} break;
					case PARSE_STAGE_RIGHT_DIGIT:
					{
						if((input[input_index] - 48) > parse.right_digit &&
							input_index > parse.left_digit_index)
						{
							parse.right_digit = input[input_index] - 48;
						}
					} break;
					case PARSE_STAGE_FINDING_NEWLINE:
					{
						parse.line_length++;
					} break;
					default:
					{
						_assert_message(0, "unexpected value for parse stage");
					} break;
				}
			} break;
			default:
			{
				log_debug("char: %c", input[input_index]);
				_assert_message(0, "unexpected char in input");
			} break;
		}
		input_index++;
	}

	log_info("joltage: %llu", parse.joltage);

	return(0);
}
