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

int main(int argc, char **argv)
{
	log_trace("C linux template");
	log_debug("C linux template");
	log_info("C linux template");
	log_warn("C linux template");
	log_error("C linux template");
	return(0);
}
