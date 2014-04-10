/*
 * mem.c
 *
 *  Created on: Apr 9, 2014
 *      Author: guru
 */


#include "include/common.h"
#include "include/perf.h"

#define BYTES_PER_KB	(1024)
#define BYTES_PER_MB	(1024 * 1024)

#define BUFFER_SIZE		(32 * BYTES_PER_MB)

static ull num_accesses = ~0;
static unsigned int stride_length = 1;

static char buffer[BUFFER_SIZE];

static int load_flag = 0, store_flag = 0;
static ull byte_idx;

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
	printf("Memory writes :%llu\n", byte_idx / stride_length);
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
	common_init();
	char *reset_buffer = malloc(2 * BYTES_PER_MB);

	if(periodic_perf) {
		printf("Identifying Stride\n");
		int repeat_idx, idx, cur_stride = 2;
		ull max_average_misses = 0;

		unsigned int max_stride = 8192;
		unsigned int reads = BUFFER_SIZE / max_stride;
		unsigned int nr_repeats = 10;

		while(cur_stride <= max_stride) {
			ull average_misses = 0, total_misses = 0;
			for(repeat_idx = 0; repeat_idx < nr_repeats; repeat_idx++) {
				for(idx = 0; idx < 2 * BYTES_PER_MB; idx++)
					reset_buffer[idx] = '0';
				reset_periodic_perf_stats();
				for(idx = 0; idx < reads; idx++) {
					buffer[cur_stride * idx] = '0';
				}
				total_misses += perf_read_periodic_perf_cache_miss();
			}
			average_misses = total_misses / nr_repeats;
			if(average_misses > max_average_misses) {
//				printf("stride %d causes more misses %llu\n", cur_stride, average_misses);
				max_average_misses = average_misses;
				stride_length = cur_stride;
			}
//			else
//				printf("stride %d causes %llu misses\n", cur_stride, average_misses);
			cur_stride *= 2;
		}
		printf("Chosen stride is :%d\n", stride_length);
	}
}

static int run_bench_mem(int argc, char **argv) {
	parse_opts(argc, argv);
	init();
	byte_idx = 0;

	signal(SIGALRM, alarm_handler);

	setitimer(ITIMER_REAL, &timeout_timer, 0);
	start_time = rdclock();
	if(periodic_perf) {
		signal(SIGUSR1, periodic_perf_handler);
		reset_periodic_perf_stats();
	}

	while(1) {
			buffer[byte_idx & BUFFER_SIZE] = '0' /*+ (byte_idx % 10)*/;
			byte_idx += stride_length;
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
