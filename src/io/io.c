/*
 * io.c
 *
 *  Created on: Jul 25, 2013
 *      Author: guru
 */

#include<include/common.h>

static int **pids;


static int run_benchmark(int argc, char **argv) {
	init_list(pids, 15);

	int i;

	for(i = 0; i < 10; i++)
		append(pids, i);

	int index = 0;
	for_each_entry(i, pids) {
		printf("list[%d] = %d\n", index, i);
		index++;
	}
}

struct benchmark io = {
	.usage				= NULL,
	.parse_opts			= NULL,
	.alarm_handler		= NULL,
	.gracefully_exit	= NULL,
	.run				= run_benchmark,
};
