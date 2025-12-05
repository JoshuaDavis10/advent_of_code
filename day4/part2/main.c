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

#define ADJACENT_POSITIONS 8
#define ADJACENT_ROLLS_THRESHOLD 4

#include "linux_util.c"

typedef struct {
	u8  adjacent_items[ADJACENT_POSITIONS];
	b32 right;
	b32 left;
	b32 top;
	b32 bottom;
	u32 grid_width; /* NOTE(josh): does not include '\n' */
	u32 grid_height;
	u32 accessible_rolls;
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

	log_trace("input: \n%s", input);
	log_trace("input size: %llu", input_size);

	/* preparse */
	u64 input_index = 0;
	while(input_index < input_size)
	{
		if(input[input_index] == '\n')
		{
			parse.grid_width = input_index;
			/* NOTE(josh): +1 for '\n' at end of each row */
			parse.grid_height = input_size / (parse.grid_width + 1); 
			log_debug("grid width:  %u", parse.grid_width);
			log_debug("grid height: %u", parse.grid_height);
			break;
		}
		input_index++;
	}

	/* actual parse */
	input_index = 0;
	while(input_index < input_size)
	{
		switch(input[input_index])
		{
			case '@':
			{
				u32 adjacent_index = 0;
				for( ; adjacent_index < ADJACENT_POSITIONS; adjacent_index++)
				{
					parse.adjacent_items[adjacent_index] = '.';
				}

				parse.left = false;
				parse.right = false;
				parse.top = false;
				parse.bottom = false;

				if(input_index < parse.grid_width) { parse.top = true; }
				if(input_index % (parse.grid_width + 1) == 0) { parse.left = true; }
				if(input_index % (parse.grid_width + 1) == (parse.grid_width - 1)) { parse.right = true; }
				if(input_index >= input_size - (parse.grid_width - 1)) { parse.bottom = true; }

				/* up and left */
				if(!parse.left && !parse.top) {
					parse.adjacent_items[0] = input[input_index - (parse.grid_width + 2)];
				}
				/* up */
				if(!parse.top) {
					parse.adjacent_items[1] = input[input_index - (parse.grid_width + 1)];
				}
				/* up and right */
				if(!parse.right && !parse.top) {
					parse.adjacent_items[2] = input[input_index - (parse.grid_width)];
				}
				/* right */
				if(!parse.right) {
					parse.adjacent_items[3] = input[input_index + 1];
				}
				/* down and right */
				if(!parse.right && !parse.bottom) {
					parse.adjacent_items[4] = input[input_index + parse.grid_width + 2];
				}
				/* down */
				if(!parse.bottom) {
					parse.adjacent_items[5] = input[input_index + parse.grid_width + 1];
				}
				/* down and left */
				if(!parse.bottom && !parse.left) {
					parse.adjacent_items[6] = input[input_index + parse.grid_width];
				}
				/* left */
				if(!parse.left) {
					parse.adjacent_items[7] = input[input_index - 1];
				}

				u32 adjacent_rolls = 0;
				adjacent_index = 0;
				for( ; adjacent_index < ADJACENT_POSITIONS; adjacent_index++)
				{
					if(parse.adjacent_items[adjacent_index] == '@')
					{
						adjacent_rolls++;
					}
				}
				if(adjacent_rolls < ADJACENT_ROLLS_THRESHOLD)
				{
					log_debug("(index: %llu): ACCESSIBLE (adjacent rolls: %u", input_index, adjacent_rolls);
					parse.accessible_rolls++;
				}
				else
				{
					log_debug("(index: %llu): INACCESSIBLE (adjacent rolls: %u", input_index, adjacent_rolls);
				}

				log_debug("adjacent items (for index: %llu):\n%c%c%c\n%c%c%c\n%c%c%c", input_index,
					parse.adjacent_items[0], parse.adjacent_items[1], parse.adjacent_items[2],
					parse.adjacent_items[7], 'I', parse.adjacent_items[3],
					parse.adjacent_items[6], parse.adjacent_items[5], parse.adjacent_items[4]);
			} break;
			case '.':
			{
				/* do nothing */
			} break;
			case '\n':
			{
				/* do nothing */
			} break;
			default:
			{
				_assert_log(false, "unexpected character during parse: %c", input[input_index]);
			} break;
		}
		input_index++;
	}

	log_info("accessible rolls: %u", parse.accessible_rolls);

	return(0);
}
