/*
 * mem.c
 *
 *  Created on: Jun 3, 2013
 *      Author: guru
 */

#include <include/common.h>

int *array;

static void usage() {
	printf("bench cpu <option>\n"
			"Benchmarks the CPU by running a primality test until conditions specified have been satisfied\n"
			"    -h    : Print this help message\n"
			"    -t    : Specifies a time limit (in seconds)\n"
			"    -n    : Number of read-write pairs to execute\n"
			);

}

static int parse_opts(int argc, char **argv) {
	int opt;

	while( (opt = getopt(argc, argv, "t:n:h")) != -1) {
		switch(opt) {
		case 't' :
			end_time = (ull) (atof(optarg) * 1e9);
			time_t sec 	= end_time / 1e9;
			time_t usec = ( (end_time - (sec * 1e9)) / 1e3);
			timeout_timer.it_interval.tv_usec = 0;
			timeout_timer.it_interval.tv_sec  = 0;
			timeout_timer.it_value.tv_sec     = sec;
			timeout_timer.it_value.tv_usec     = usec;
			break;
		case 'n' :
			end_number = (ull) atol(optarg);
			break;
		default :
			usage();
			break;
		}
	}
	return 0;
}

static int gracefully_exit() {
	printf("Time elapsed         :%0.6fs\n", ((finish_time - start_time) / 1e9) );
	printf("Read-writes executed :%llu\n", finish_number);
	exit(0);
}

static void alarm_handler(int signal) {
	finish_time = rdclock();
	finish_number = current_number;
	gracefully_exit();
//	Should have already exited..
	printf("Graceful exit did not exit!?\n");
	exit(-1);
}






static int bench_mem(int argc, char **argv) {
	parse_opts(argc, argv);

	if(end_time == ~0 && end_number == ~0) {
		bzero(error_msg, strlen(error_msg));
		sprintf(error_msg, "No options specified\nTerminating program due to infinite loop!\n");
		panic();
	}

	array = malloc(sizeof(DCACHE_SIZE) * 2 + 1);

	signal(SIGALRM, alarm_handler);

	setitimer(ITIMER_REAL, &timeout_timer, 0);
	start_time = rdclock();

	ull index = 0;
	while(1) {
		memcpy(&array[index], &array[index + DCACHE_SIZE], sizeof(int));

		if(current_number == end_number)
			break;

		if(index == DCACHE_SIZE)
			index = 0;

		current_number++;
		index++;
	}

//	Manual termination
	alarm_handler(SIGALRM);
}

struct benchmark mem = {
	.usage				= usage,
	.parse_opts			= parse_opts,
	.alarm_handler		= alarm_handler,
	.gracefully_exit	= gracefully_exit,
	.run				= bench_mem,
};
