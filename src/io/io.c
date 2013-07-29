/*
 * io.c
 *
 *  Created on: Jul 25, 2013
 *      Author: guru
 */

#include<include/common.h>

static struct list *pids;


static int run_benchmark(int argc, char **argv) {
	init_list(pids);

	int i;

	for(i = 0; i < 5; i++) {
		append(pids, i);
		printf("list->last_ptr = %X\n", pids->last_ptr);
	}

	int index = 0;
	int *entry;
	for_each_entry(entry, pids) {
		printf("list->cur_ptr  = %X\n", __node);
		printf("list[%d] = %d\n", index, *entry);
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
