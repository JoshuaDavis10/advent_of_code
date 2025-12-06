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

	/* TODO(josh): so I think I want to store the ranges in a binary tree 
	 * - if the range is lower than it's parent, it's a left child, if the range is higher than it's parent,
	 *   it's a right child... XXX do we want to do any balancing? that seems somewhat unnecessary
	 *   XXX: if you are trying to find where in the tree a range should go, and you realize that it is a superset
	 *   or subset of a node, then delete that node, or the range, respectively... if its the superset case
	 *   you may have to also delete children, if they also a subset of said superset.
	 * - that's a good bit of initial setup, but it should make the lookups much faster than a naive 
	 *   implementation ( you could even do a naive implementation and compare )
	 */

	u64 input_index = 0;
	while(input_index < input_size)
	{
		switch(input[input_index])
		{
			/* XXX: */
			default:
			{
			} break;
		}
		input_index++;
	}

	return(0);
}
