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

/* NOTE(josh): arbitrary max digit count */
#define MAX_DIGIT_COUNT 10

enum {
	DIAL_LEFT,
	DIAL_RIGHT,
	DIAL_UNKNOWN
};

int main(int argc, char **argv)
{
	if(argc != 2)
	{
		log_error("usage -> ./prog [input]");
		return(-1);
	}

	i32 dial_position = 50;
	u32 dial_at_zero_count = 0;
	u64 input_size = get_file_size(argv[1]);
	char *input = (char*)read_file_mmapped(argv[1]);

	log_trace("input: '%s'", input);

	u64 input_index = 0;
	i32 dial_direction = DIAL_UNKNOWN;
	i32 digit_count = 0;
	i32 turn_distance = 0;
	while(input_index < input_size)
	{
		switch(input[input_index])
		{
			case 'L':
			{
				dial_direction = DIAL_LEFT;	
			} break;
			case 'R':
			{
				dial_direction = DIAL_RIGHT;	
			} break;
			case '0':
			{
				_assert(digit_count != 0);
				turn_distance *= 10;
				digit_count++;
				_assert(digit_count < MAX_DIGIT_COUNT);
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
				i32 digit = input[input_index] - 48; 
				turn_distance = turn_distance * 10 + digit; 
				digit_count++;
				_assert(digit_count < MAX_DIGIT_COUNT);
			} break;
			case '\n':
			{
				/* TODO: issue whatever dial turn you've gotten */
				switch(dial_direction)
				{
					case DIAL_LEFT:
					{
						dial_position -= turn_distance;
						dial_position = dial_position % 100;
					} break;
					case DIAL_RIGHT:
					{
						dial_position += turn_distance;
						dial_position = dial_position % 100;
					} break;
					default:
					{
						log_error("did not get a valid dial direction to turn");
						_assert(0);
					} break;
				}

				if(dial_position == 0)
				{
					log_debug("direction: %d, distance: %d (landed @ 0)", dial_direction, turn_distance);
					dial_at_zero_count++;
				}

				digit_count = 0;
				turn_distance = 0;
				dial_direction = DIAL_UNKNOWN;
			} break;
			default:
			{
				log_error("unexpected char: '%c'", input[input_index]);
				_assert(0);
			} break;
		}
		input_index++;
	}

	log_info("times dial hit zero: %u", dial_at_zero_count);

	return(0);
}
