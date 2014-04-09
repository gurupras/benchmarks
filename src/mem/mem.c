/*
 * mem.c
 *
 *  Created on: Apr 9, 2014
 *      Author: guru
 */


#include <include/common.h>
#include <fcntl.h>

#define BYTES_PER_KB	1024
#define BYTES_PER_MB	1024 * 1024

#define BUFFER_SIZE		4 * BYTES_PER_MB

static ull num_accesses = ~0;
static int stride_length = 32 * BYTES_PER_KB;

static volatile char buffer1[BUFFER_SIZE];
static volatile char buffer2[BUFFER_SIZE];

static int load_flag = 0, store_flag = 0;


static void usage(char *error) {
	struct _IO_FILE *file = stdout;

	if(strcmp(error, " ") != 0)
		file = stderr;
	fprintf(file, "%s\n"
			"bench mem <option>\n"
			"Benchmarks the memory by executing loads and stores\n"
			"\nOPTIONS:\n"
			"    -h        : Print this help message\n"
			"    -t <n>    : Specifies a time limit (in nanoseconds)\n"
			"    -n <n>    : Specifies number of memory accesses(loads and stores)\n"
			"    -l        : UNIMPLEMENTED: Use memory loads only (default is mixed)\n"
			"    -s        : UNIMPLEMENTED: Use memory stores only (default is mixed)\n"
			"    -d <n>    : Set stride length to <n> bytes\n"
			"    -p        : Enable performance counter accounting\n"
			"\nNOTE:\n"
			"At least one of -t or -n must be set. \n"
			"If unset, the program terminates as there is no break condition\n"
			"If both are set, the program terminates upon satisfying either condition\n"
			, error);

	if(strcmp(error, " ") != 0)
		exit(-1);
}

static int parse_opts(int argc, char **argv) {
	int opt;

	while( (opt = getopt(argc, argv, "t:n:d:hlsp")) != -1) {
		switch(opt) {
		case ':' :
			usage("missing parameter value");
			break;
		case 't' : {
			end_time = (ull) (atof(optarg) * 1e9);
			time_t sec 	= end_time / 1e9;
			time_t usec = ( (end_time - (sec * 1e9)) / 1e3);
			timeout_timer.it_interval.tv_usec = 0;
			timeout_timer.it_interval.tv_sec  = 0;
			timeout_timer.it_value.tv_sec     = sec;
			timeout_timer.it_value.tv_usec     = usec;
			break;
		}
		case 'n' :
			num_accesses = strtoull(optarg, 0, 0);
			break;
		case 'l' :
			load_flag = 1;
			break;
		case 's' :
			store_flag = 1;
			break;
		case 'd' :
			stride_length = atoi(optarg);
			break;
		case 'h' :
			usage(" ");
			break;
		case 'p' :
//			Experimental periodic_perf
			periodic_perf = 1;
			break;
		default :
		case '?' :
			usage("invalid command line argument");
			break;
		}
	}

	if(load_flag && store_flag)
		printf("Warning: loads and stores are both selected. Switching to mixed mode\n");
	if(!load_flag && !store_flag) {
		load_flag = 1;
		store_flag = 1;
	}
	if(end_time == 0 && num_accesses == ~0) {
		usage("No options specified\nTerminating program due to infinite loop!\n");
		panic(" ");
	}

	return 0;
}

static int gracefully_exit() {
	printf("Time elapsed  :%0.6fs\n", ((finish_time - start_time) / 1e9) );

	if(periodic_perf) {
		periodic_perf_handler(SIGUSR1);
	}
	exit(0);
}

static void alarm_handler(int signal) {
	finish_time = rdclock();
	gracefully_exit();
}

static void init() {
	int idx;
	srand(time(NULL));
	for(idx = 0; idx < BUFFER_SIZE; idx++) {
		buffer1[idx] = rand() % 256;
		buffer2[idx] = rand() % 256;
	}
}

static int run_bench_mem(int argc, char **argv) {
	parse_opts(argc, argv);
	init();

	signal(SIGALRM, alarm_handler);

	if(periodic_perf) {
		signal(SIGUSR1, periodic_perf_handler);
		reset_periodic_perf_stats();
	}

	setitimer(ITIMER_REAL, &timeout_timer, 0);
	start_time = rdclock();

	int buffer_pos = 0;
	while(1) {
		buffer1[buffer_pos] = buffer2[buffer_pos];

		buffer_pos = (buffer_pos + stride_length) % BUFFER_SIZE;
//		access_idx++;
//		if(unlikely(access_idx == num_accesses))
//			alarm_handler(SIGALRM);
	}

	return 0;
}

struct benchmark mem = {
	.usage				= NULL,
	.parse_opts			= NULL,
	.alarm_handler		= NULL,
	.gracefully_exit	= NULL,
	.run				= run_bench_mem,
};
