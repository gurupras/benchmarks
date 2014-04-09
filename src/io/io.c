/*
 * io.c
 *
 *  Created on: Jul 25, 2013
 *      Author: guru
 */

#include<include/common.h>

static int run_benchmark(int argc, char **argv) {
	return 0;
}

struct benchmark io = {
	.usage				= NULL,
	.parse_opts			= NULL,
	.alarm_handler		= NULL,
	.gracefully_exit	= NULL,
	.run				= run_benchmark,
};
