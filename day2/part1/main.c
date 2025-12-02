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

/* NOTE(josh): largest unsigned 64 bit integer is: 18,446,744,073,709,551,615, with 20 digits 
 * so we would need a 20 bytes string to store it (one byte per digit). so all IDs will be 
 * able to be stored within a 20 byte string */
#define MAX_ID_STRING_LENGTH 20

static char *global_id_string_buffer = 0;

enum {
	PARSE_STATE_FIRST_NUMBER,
	PARSE_STATE_SECOND_NUMBER,
	PARSE_STATE_COUNT
};

b32 is_id_invalid(u64 id)
{
	u32 digits = 0;
	u64 temp = id;

	/* NOTE(josh): this is filling the buffer backwards, but we don't care bc all we have to do
	 * is compare the 2 havles to see if they are equal */
	while(temp != 0)
	{
		/* NOTE(josh): +48 so output is at least human readable for debugging, even tho it's backwards */
		global_id_string_buffer[digits] = (temp % 10) + 48; 
		temp = temp / 10;
		digits++;
	}
	global_id_string_buffer[digits] = '\0';

	/* odd IDs are always valid */
	if(digits % 2 == 1)
	{
		return(false);
	}

	b32 equal_halves = true;
	u32 midpoint_index = digits / 2;
	u32 index = 0;
	while(index < midpoint_index)
	{
		if(global_id_string_buffer[index] != global_id_string_buffer[index + midpoint_index])
		{
			equal_halves = false;
			break;
		}
		index++;
	}

	return(equal_halves);
}

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

	global_id_string_buffer = malloc(MAX_ID_STRING_LENGTH + 1); /* NOTE(josh): +1 for null terminator */
	_assert(global_id_string_buffer);

	u64 invalid_id_accumulator = 0;
	u64 range_start = 0;
	u64 range_end = 0;
	u32 digits = 0;
	i32 parse_state = PARSE_STATE_FIRST_NUMBER;
	u64 input_index = 0;
	while(input_index < input_size && input[input_index])
	{
		switch(input[input_index])
		{
			case '\n':
			case ',':
			{
				_assert(parse_state == PARSE_STATE_SECOND_NUMBER);
				u64 id = 0;
				for(id = range_start ; id <= range_end; id++)
				{
					if(is_id_invalid(id))
					{
						invalid_id_accumulator += id;
					}
				}
				range_start = 0;
				range_end = 0;
				digits = 0;
				parse_state = PARSE_STATE_FIRST_NUMBER;
			} break;
			case '-':
			{
				_assert(parse_state == PARSE_STATE_FIRST_NUMBER);
				parse_state = PARSE_STATE_SECOND_NUMBER;
				digits = 0;
			} break;
			case '0':
			{
				_assert(digits > 0); /* NOTE(josh): if 0 is first digit, then input is invalid */
				switch(parse_state)
				{
					case PARSE_STATE_FIRST_NUMBER:
					{
						range_start = range_start * 10;
					} break;
					case PARSE_STATE_SECOND_NUMBER:
					{
						range_end = range_end * 10;
					} break;
					default:
					{
						log_error("unexpected parse state during parse: %d", parse_state);
						_assert(0);
					}
				}
				digits++;
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
				switch(parse_state)
				{
					case PARSE_STATE_FIRST_NUMBER:
					{
						range_start = (range_start * 10) + (input[input_index] - 48);
					} break;
					case PARSE_STATE_SECOND_NUMBER:
					{
						range_end = (range_end * 10) + (input[input_index] - 48);
					} break;
					default:
					{
						log_error("unexpected parse state during parse: %d", parse_state);
						_assert(0);
					}
				}
				digits++;
			} break;
			default:
			{
				log_error("unexpected char during parse: %c", input[input_index]);
				_assert(0);
			} break;
		}
		input_index++;
	}

	log_info("sum of invalid ids: %llu", invalid_id_accumulator);

	return(0);
}
