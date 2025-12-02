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
	u32 dial_pass_zero_count = 0;
	u64 input_size = get_file_size(argv[1]);
	char *input = (char*)read_file_mmapped(argv[1]);

	u64 input_index = 0;
	i32 dial_direction = DIAL_UNKNOWN;
	i32 digit_count = 0;
	i32 turn_distance = 0;
	while(input_index < input_size)
	{
		log_trace("%c", input[input_index]);
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
				switch(dial_direction)
				{
					case DIAL_LEFT:
					{
						/* NOTE(josh): turning a distance of 100 will always pass a 0 */
						log_debug("moving LEFT %d units (dial position: %d)", turn_distance, dial_position);
						log_debug("pass zero count b4 move: %u", dial_pass_zero_count);
						dial_pass_zero_count += turn_distance / 100;
						i32 turn_distance_remaining = turn_distance % 100;

						if(turn_distance_remaining >= dial_position && dial_position != 0)
						{
							dial_pass_zero_count++;
						}

						log_debug("pass zero count after move: %u", dial_pass_zero_count);

						dial_position -= turn_distance;
						if(dial_position < 0)
						{
							dial_position = (100 + (dial_position % 100)) % 100;
						}
						else
						{
							dial_position = dial_position % 100;
						}
						log_debug("new dial position: %d", dial_position);
					} break;
					case DIAL_RIGHT:
					{
						/* NOTE(josh): turning a distance of 100 will always pass a 0 */
						log_debug("moving RIGHT %d units (dial position: %d)", turn_distance, dial_position);
						log_debug("pass zero count b4 move: %u", dial_pass_zero_count);
						dial_pass_zero_count += (dial_position + turn_distance) / 100;

						log_debug("pass zero count after move: %u", dial_pass_zero_count);

						dial_position += turn_distance;
						if(dial_position < 0)
						{
							/* NOTE(josh): this never happens right ? */
							dial_position = 100 + (dial_position % 100);
						}
						else
						{
							dial_position = dial_position % 100;
						}
						log_debug("new dial position: %d", dial_position);
					} break;
					default:
					{
						log_error("did not get a valid dial direction to turn");
						_assert(0);
					} break;
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

	log_info("times dial passed zero: %u", dial_pass_zero_count);

	return(0);
}
