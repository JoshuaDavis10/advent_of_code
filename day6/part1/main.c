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

enum {
	/* XXX: */
	PARSE_STAGE_COUNT
};

typedef struct {
	/* XXX: */
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

	log_trace("input: \n%s", input);
	log_trace("input size: %llu", input_size);

	/* TODO: here's my plan
	 * - preparse to find out how many problems there are (i.e. # of spacebars b4 newline). only have to parse first line, nice.
	 * - create that many dynamic arrays to store the numbers for each problem, and just add to those arrays as u scan numbers
	 * - XXX: if you didn't want the arrays to be dynamic and instead want to allocate all memory up front, you'd have to do a full
	 *   preparse to find out how many lines there are. I guess that's an option if you want. less mem allocation but a longer
	 *   parse.
	 * - once you've read in numbers for all problems and operation (* or +) for each problem then do the problems and add result
	 */

	u64 input_index = 0;
	while(input_index < input_size)
	{
		switch(input[input_index])
		{
			case '\n':
			{
				/* XXX: */
			} break;
			case '*':
			{
				/* XXX: */
			} break;
			case '+':
			{
				/* XXX: */
			} break;
			case ' ':
			{
				/* XXX: */
			} break;
			case '0':
			{
				/* XXX: */
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
				/* XXX: */
			} break;
			default:
			{
				_assert_log(0, "unexpected char during parse: %c (%x)", input[input_index], input[input_index]);
			} break;
		}
		input_index++;
	}

	u64 grand_total = 0;

	log_info("GRAND TOTAL: %llu", grand_total);

	return(0);
}
