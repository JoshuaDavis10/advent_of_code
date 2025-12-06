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

/* NOTE(josh): arbitrary cap to prevent the program from using way too much memory */
/* XXX(josh): note that the memory is not very packed bc the trees are probably very not balanced, so I'm using like
 * 5x as much memory as a balanced tree would (like one of the nodes is @ index 1503, so lots of those indexes are unused)
 * at some point I'll look into writing a balanced binary tree, just not gonna do it here 
 */

#define MAX_ID_RANGE_TREE_NODES 1600

typedef struct {
	u64 range_minimum;
	u64 range_maximum;
	b32 parsing_maximum; /* if false, then we are parsing the minimum */
	b32 last_char_was_newline;
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

static void handle_overlap(i32 node_index, i32 top_level_index, u64 *top_level_min, u64 *top_level_max);
static void delete_node(i32 node_index);

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
		/* no overlap */
		if(max < current_node.range_minimum && min < current_node.range_minimum)
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
					if(insert_index > MAX_ID_RANGE_TREE_NODES)
					{
						_assert_log(0, "we hit our arbitrary node count limit (%d), while trying to insert at index %d",
							MAX_ID_RANGE_TREE_NODES, insert_index);
					}
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
		/* no overlap */
		else if(max > current_node.range_maximum && min > current_node.range_maximum)
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
		/* overlap */
		else
		{
			handle_overlap(current_node_index, current_node_index, &min, &max);
			global_id_ranges[current_node_index].range_minimum = min;
			global_id_ranges[current_node_index].range_maximum = max;
			log_debug("replaced node @ index %d with range (%llu, %llu)", current_node_index, 
				global_id_ranges[current_node_index].range_minimum, global_id_ranges[current_node_index].range_maximum);
			break;
		}
	}
}

static void handle_overlap(i32 node_index, i32 top_level_index, u64 *top_level_min, u64 *top_level_max)
{
	u64 node_min = global_id_ranges[node_index].range_minimum;
	u64 node_max = global_id_ranges[node_index].range_maximum;

	log_debug("handle_overlap: node %d, (%llu, %llu). top level: (%llu, %llu)", node_index, node_min, node_max, 
		   *top_level_min, *top_level_max);
	/* no overlap, node is on left of top level range */
	if(node_min < *top_level_min && node_max < *top_level_min)
	{
		/* check if right child has overlap */
		if(global_id_ranges[node_index].has_right_child)
		{
			handle_overlap(node_index * 2 + 2, node_index, top_level_min, top_level_max);
		}
	}

	/* no overlap, node is on right of top level range */
	else if(node_min > *top_level_max && node_max > *top_level_max)
	{
		/* check if left child has overlap */
		if(global_id_ranges[node_index].has_left_child)
		{
			handle_overlap(node_index * 2 + 1, node_index, top_level_min, top_level_max);
		}
	}

	/* overlap, node range is left skewed compared to top level range */
	else if(node_min < *top_level_min && node_max < *top_level_max && node_max >= *top_level_min)
	{
		*top_level_min = node_min;
		log_debug("handle_overlap: node %d, (%llu, %llu). top level CHANGED: (%llu, %llu)", node_index, node_min, node_max, 
		   *top_level_min, *top_level_max);

		/* check if right child has overlap */
		if(global_id_ranges[node_index].has_right_child)
		{
			handle_overlap(node_index * 2 + 2, node_index, top_level_min, top_level_max);
		}
		if(node_index != top_level_index)
		{
			log_debug("handle_overlap: deleting node %d", node_index);
			delete_node(node_index);
		}
	}

	/* overlap, node range is right skewed compared to top level range */
	else if(node_min > *top_level_min && node_max > *top_level_max && node_min <= *top_level_max)
	{
		*top_level_max = node_max;
		log_debug("handle_overlap: node %d, (%llu, %llu). top level CHANGED: (%llu, %llu)", node_index, node_min, node_max, 
		   *top_level_min, *top_level_max);

		/* check if left child has overlap */
		if(global_id_ranges[node_index].has_left_child)
		{
			handle_overlap(node_index * 2 + 1, node_index, top_level_min, top_level_max);
		}
		if(node_index != top_level_index)
		{
			log_debug("handle_overlap: deleting node %d", node_index);
			delete_node(node_index);
		}
	}

	/* overlap, node is a superset, so it is the top level node, so set top level min/max to this node's values */
	else if(node_min <= *top_level_min && node_max >= *top_level_max)
	{
		*top_level_min = node_min;
		*top_level_max = node_max;
	}

	/* overlap, node is a subset */
	else if(node_min >= *top_level_min && node_max <= *top_level_max)
	{
		/* check if left child has overlap */
		if(global_id_ranges[node_index].has_left_child)
		{
			handle_overlap(node_index * 2 + 1, node_index, top_level_min, top_level_max);
		}
		/* check if right child has overlap */
		if(global_id_ranges[node_index].has_right_child)
		{
			handle_overlap(node_index * 2 + 2, node_index, top_level_min, top_level_max);
		}
		if(node_index != top_level_index)
		{
			log_debug("handle_overlap: deleting node %d", node_index);
			delete_node(node_index);
		}
	}

	else {
		_assert_log(0, "hit unexpected handle overlap case. ranges: (%llu, %llu), (%llu, %llu)",
			  node_min, node_max, *top_level_min, *top_level_max);
	}
}

static u64 get_fresh_id_count(i32 node_index)
{
	u64 min = global_id_ranges[node_index].range_minimum;
	u64 max = global_id_ranges[node_index].range_maximum;
	u64 fresh_ids = max - min + 1;

	log_debug("node %d: (%llu, %llu) -> fresh ids = %llu", node_index, min, max, fresh_ids); 

	u64 result = fresh_ids;
	if(global_id_ranges[node_index].has_left_child)
	{
		result += get_fresh_id_count(node_index * 2 + 1);
	}
	if(global_id_ranges[node_index].has_right_child)
	{
		result += get_fresh_id_count(node_index * 2 + 2);
	}

	return(result);
}

static void delete_node(i32 node_index)
{
	id_range_tree_node *node = &(global_id_ranges[node_index]);
	if(node->has_right_child)
	{
		id_range_tree_node right_child = global_id_ranges[node_index * 2 + 2];
		node->range_minimum = right_child.range_minimum;
		node->range_maximum = right_child.range_maximum;
		delete_node(node_index * 2 + 2);
	}
	else if(node->has_left_child)
	{
		id_range_tree_node left_child = global_id_ranges[node_index * 2 + 1];
		node->range_minimum = left_child.range_minimum;
		node->range_maximum = left_child.range_maximum;
		delete_node(node_index * 2 + 1);
	}
	else
	{
		/* if our index is odd, we are a left child */
		if(node_index % 2 == 1)
		{
			i32 parent_index = (node_index - 1 ) / 2;
			global_id_ranges[parent_index].has_left_child = false;
		}
		else
		{
			i32 parent_index = (node_index - 2 ) / 2;
			global_id_ranges[parent_index].has_right_child = false;
		}
	}
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
	 * - OKAY, SO BASICALLY WHILE PARSING, IF 2 RANGES OVERLAP, YOU NEED TO MERGE THEM INTO ONE RANGE
	 */
	u64 fresh_ids = 0;
	u64 input_index = 0;
	b32 parsing = true;
	while(input_index < input_size && parsing)
	{
		switch(input[input_index])
		{
			case '\n':
			{
				{
					if(global_parse_data.last_char_was_newline)
					{
						parsing = false;
					}
					else
					{
						add_id_range(global_parse_data.range_minimum, global_parse_data.range_maximum);
						global_parse_data.range_minimum = 0;
						global_parse_data.range_maximum = 0;
						global_parse_data.parsing_maximum = false;
						global_parse_data.last_char_was_newline = true;
					}
				}
			} break;
			case '-':
			{
				_assert(global_parse_data.parsing_maximum == false);
				global_parse_data.parsing_maximum = true;
				global_parse_data.last_char_was_newline = false;
			} break;
			case '0':
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
			default:
			{
				_assert_log(0, "parse encountered unexpected char: %c", input[input_index]);
			} break;
		}
		input_index++;
	}

	fresh_ids = get_fresh_id_count(0);

	log_info("fresh ids: %llu", fresh_ids);

	return(0);
}
