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
	PARSE_STAGE_GETTING_RANGES,
	PARSE_STAGE_CHECKING_IDS,
	PARSE_STAGE_COUNT
};

typedef struct {
	u64 range_minimum;
	u64 range_maximum;
	u64 id;
	b32 parsing_maximum; /* if false, then we are parsing the minimum */
	b32 last_char_was_newline;
	i32 parse_stage;
} parse_data;

typedef struct {
	u64 range_minimum;
	u64 range_maximum;
	b32 has_left_child;
	b32 has_right_child;
} id_range_tree_node;

static parse_data global_parse_data;
static id_range_tree_node *global_id_ranges = 0;
static u32 global_nodes_allocated = 0;

static void add_id_range(u64 min, u64 max)
{
	/* NOTE(josh): only occurs when inserting first node */
	if(global_id_ranges[0].range_minimum == 0 && global_id_ranges[0].range_maximum == 0)
	{
		_assert(global_nodes_allocated > 0);
		global_id_ranges[0].range_minimum = min;
		global_id_ranges[0].range_maximum = max;
		log_debug("created root node with range (%llu, %llu)", 
			global_id_ranges[0].range_minimum, global_id_ranges[0].range_maximum);
		return;
	}
	i32 current_node_index = 0;
	b32 done = false;
	while(1)
	{
		id_range_tree_node current_node = global_id_ranges[current_node_index];
		if(min > current_node.range_minimum && max > current_node.range_maximum)
		{
			if(current_node.has_right_child)
			{
				current_node_index = 2 * current_node_index + 2;
			}
			else
			{
				i32 insert_index = 2 * current_node_index + 2;
				if(insert_index >= global_nodes_allocated)
				{
					global_id_ranges = realloc(global_id_ranges, insert_index * 2 * sizeof(id_range_tree_node));
					_assert(global_id_ranges);
					global_nodes_allocated = insert_index * 2;
				}
				global_id_ranges[insert_index].range_minimum = min;
				global_id_ranges[insert_index].range_maximum = max;
				global_id_ranges[insert_index].has_left_child = false;
				global_id_ranges[insert_index].has_right_child = false;
				global_id_ranges[current_node_index].has_right_child = true;
				log_debug("created node @ index %d with range (%llu, %llu)", insert_index, 
					global_id_ranges[insert_index].range_minimum, global_id_ranges[insert_index].range_maximum);
				break;
			}
		}
		else if(min < current_node.range_minimum && max < current_node.range_maximum)
		{
			if(current_node.has_left_child)
			{
				current_node_index = 2 * current_node_index + 1;
			}
			else
			{
				i32 insert_index = 2 * current_node_index + 1;
				if(insert_index >= global_nodes_allocated)
				{
					global_id_ranges = realloc(global_id_ranges, insert_index * 2 * sizeof(id_range_tree_node));
					_assert(global_id_ranges);
					global_nodes_allocated = insert_index * 2;
				}
				global_id_ranges[insert_index].range_minimum = min;
				global_id_ranges[insert_index].range_maximum = max;
				global_id_ranges[insert_index].has_left_child = false;
				global_id_ranges[insert_index].has_right_child = false;
				global_id_ranges[current_node_index].has_left_child = true;
				log_debug("created node @ index %d with range (%llu, %llu)", insert_index, 
					global_id_ranges[insert_index].range_minimum, global_id_ranges[insert_index].range_maximum);
				break;
			}
		}
		else
		{
			if(min >= current_node.range_minimum && max <= current_node.range_maximum)
			{
				/* do nothing since it's a repeat range or subset */
				break;
			}
			/* XXX: okay I'm just gonna replace the node with the superset one, that should still work yeah ? */
			log_debug("replacing range (%llu, %llu) with range (%llu, %llu)", 
				current_node.range_minimum, current_node.range_maximum,
				min, max);
			global_id_ranges[current_node_index].range_minimum = min;
			global_id_ranges[current_node_index].range_maximum = max;
			break;
			/*
			_assert_log(0, "looks like we hit the case where a range is a subset of another. the 2 ranges "
				"involved are: (%llu, %llu) and (%llu, %llu).",
				current_node.range_minimum, current_node.range_maximum,
				min, max);
				*/
		}
	}
}

static b32 is_ingredient_fresh(u64 ingredient_id)
{
	b32 fresh = false;
	b32 done = false;
	i32 current_node_index = 0;
	while(1)
	{
		id_range_tree_node current_node = global_id_ranges[current_node_index];
		if(ingredient_id >= current_node.range_minimum && ingredient_id <= current_node.range_maximum)
		{
			log_debug("ingredient %llu is fresh!", ingredient_id);
			fresh = true;
			break;
		}

		if(ingredient_id > current_node.range_maximum)
		{
			/* go to right child */
			if(current_node.has_right_child)
			{
				current_node_index = 2 * current_node_index + 2;
				continue;
			}
			else
			{
				break;
			}
		}

		if(ingredient_id < current_node.range_minimum)
		{
			/* go to right child */
			if(current_node.has_left_child)
			{
				current_node_index = 2 * current_node_index + 1;
				continue;
			}
			else
			{
				break;
			}
		}

	}

	return(fresh);
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

	log_trace("input: \n%s", input);
	log_trace("input size: %llu", input_size);

	global_id_ranges = malloc(sizeof(id_range_tree_node) * 3);
	_assert(global_id_ranges);
	global_id_ranges[0].has_left_child = false;
	global_id_ranges[0].has_right_child = false;
	global_id_ranges[0].range_minimum = 0;
	global_id_ranges[0].range_maximum = 0;
	global_nodes_allocated = 3;

	/* TODO: day 5 part 2 stuff 
	 * - only checking ranges now, so no reading in ids
	 * - I think you'll have to leverage the binary tree structure in some way
	 *		have each node ask it's children how many unique ids it has in its range by giving each child
	 *		the nodes min/max and it's siblings min/max ?
	 *		then each node sort of percolates that up to the root ?
	 */
	u32 fresh_ids = 0;
	u64 input_index = 0;
	while(input_index < input_size)
	{
		switch(input[input_index])
		{
			case '\n':
			{
				switch(global_parse_data.parse_stage)
				{
					case PARSE_STAGE_GETTING_RANGES:
					{
						if(global_parse_data.last_char_was_newline)
						{
							global_parse_data.parse_stage = PARSE_STAGE_CHECKING_IDS;
						}
						else
						{
							add_id_range(global_parse_data.range_minimum, global_parse_data.range_maximum);
							global_parse_data.range_minimum = 0;
							global_parse_data.range_maximum = 0;
							global_parse_data.parsing_maximum = false;
							global_parse_data.last_char_was_newline = true;
						}
					} break;
					case PARSE_STAGE_CHECKING_IDS:
					{
						if(is_ingredient_fresh(global_parse_data.id))
						{
							fresh_ids++;
						}
						global_parse_data.id = 0;
					} break;
					default:
					{
						_assert_log(0, "unexpected parse stage encountered: %d", global_parse_data.parse_stage);
					}
				}
			} break;
			case '-':
			{
				_assert(global_parse_data.parse_stage == PARSE_STAGE_GETTING_RANGES);
				_assert(global_parse_data.parsing_maximum == false);
				global_parse_data.parsing_maximum = true;
				global_parse_data.last_char_was_newline = false;
			} break;
			case '0':
			{
				switch(global_parse_data.parse_stage)
				{
					case PARSE_STAGE_GETTING_RANGES:
					{
						if(global_parse_data.parsing_maximum)
						{
							/* NOTE(josh) making sure we don't ever start with a lone zero */
							_assert(global_parse_data.range_maximum != 0); 
							global_parse_data.range_maximum = global_parse_data.range_maximum * 10; 
						}
						else
						{
							/* NOTE(josh) making sure we don't ever start with a lone zero */
							_assert(global_parse_data.range_minimum != 0); 
							global_parse_data.range_minimum = global_parse_data.range_minimum * 10; 
						}
						global_parse_data.last_char_was_newline = false;
					} break;
					case PARSE_STAGE_CHECKING_IDS:
					{
						/* NOTE(josh) making sure we don't ever start with a lone zero */
						_assert(global_parse_data.id != 0); 
						global_parse_data.id = global_parse_data.id * 10;
					} break;
					default:
					{
						_assert_log(0, "unexpected parse stage encountered: %d", global_parse_data.parse_stage);
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
				switch(global_parse_data.parse_stage)
				{
					case PARSE_STAGE_GETTING_RANGES:
					{

						if(global_parse_data.parsing_maximum)
						{
							global_parse_data.range_maximum = global_parse_data.range_maximum * 10 + 
								(input[input_index] - 48);
						}
						else
						{
							global_parse_data.range_minimum = global_parse_data.range_minimum * 10 + 
								(input[input_index] - 48);
						}
						global_parse_data.last_char_was_newline = false;
					} break;
					case PARSE_STAGE_CHECKING_IDS:
					{
						global_parse_data.id = global_parse_data.id * 10 + (input[input_index] - 48);
					} break;
					default:
					{
						_assert_log(0, "unexpected parse stage encountered: %d", global_parse_data.parse_stage);
					} break;
				}
			} break;
			default:
			{
				_assert_log(0, "parse encountered unexpected char: %c", input[input_index]);
			} break;
		}
		input_index++;
	}

	log_info("fresh ids: %u", fresh_ids);

	return(0);
}
