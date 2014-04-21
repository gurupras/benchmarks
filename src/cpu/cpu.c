/*
 * cpu.c
 *
 *  Created on: May 31, 2013
 *      Author: guru
 */

#include "include/common.h"
#include "include/perf.h"
#include "include/tuning_library.h"

u64 end_number = ~0, current_number = 0, finish_number = -1;
static int is_tuning_enabled = 0;
static struct timespec sleep_interval = {0,0};
static struct list *primes;

static void usage(char *error) {
	struct _IO_FILE *file = stdout;

	if(strcmp(error, " ") != 0)
		file = stderr;
	fprintf(file, "%s\n"
			"bench cpu <option>\n"
			"Benchmarks the CPU by running a primality test until conditions specified have been satisfied\n"
			"\nOPTIONS:\n"
			"    -h        : Print this help message\n"
			"    -t <n>    : Specifies a time limit (in nanoseconds)\n"
			"    -s <n>    : Specifies an intermittent sleep interval\n"
			"    -n <n>    : Specifies a number limit\n"
			"    -p        : Enable performance counter accounting\n"
			"    -u        : Enable the power-agile tuning library\n"
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

	while( (opt = getopt(argc, argv, "t:n:s:r:hpu")) != -1) {
		switch(opt) {
		case ':' :
			usage("missing parameter value");
			break;
		case 't' : {
			end_time = (u64) (atof(optarg) * 1e9);
			time_t sec 	= end_time / 1e9;
			time_t usec = ( (end_time - (sec * 1e9)) / 1e3);
			timeout_timer.it_interval.tv_usec = 0;
			timeout_timer.it_interval.tv_sec  = 0;
			timeout_timer.it_value.tv_sec     = sec;
			timeout_timer.it_value.tv_usec     = usec;
			break;
		}
		case 's' : {
			time_t sleep_period = (u64) (atof(optarg) * 1e9);
			sleep_interval.tv_sec	= sleep_period / 1e9;
			sleep_interval.tv_nsec	= (sleep_period - (sleep_interval.tv_sec * 1e9));
			break;
		}
		case 'n' :
			end_number = strtoull(optarg, 0, 0);
			break;
		case 'h' :
			usage(" ");
			break;
		case 'p' :
//			Experimental periodic_perf
			periodic_perf = 1;
			break;
		case 'u' :
			is_tuning_enabled = 1;
			break;
		default :
		case '?' :
			usage("invalid command line argument");
			break;
		}
	}

	if(end_time == 0 && end_number == ~0) {
		usage("No options specified\nTerminating program due to infinite loop!\n");
		panic(" ");
	}

	return 0;
}

static int gracefully_exit() {
	printf("Time elapsed  :%0.6fs\n", ((finish_time - start_time) / 1e9) );
	printf("Last number   :%llu\n", finish_number);

	if(periodic_perf) {
		perf_handler(SIGUSR1);
	}
	exit(0);
}

static void alarm_handler(int signal) {
	finish_time = rdclock();
	finish_number = current_number;
	gracefully_exit();
}

static inline int is_prime(int number) {

    unsigned int divisor = 3;

    if(number == 1ULL ) {
        //1 is neither prime nor composite
        return 0;
    }

    if(number % 2 == 0) {
        return 0; //even no.
    }

//    Every prime number is of the form 6n (+|-) 1
    if(number > 6 && (number % 6 != 1 || number % 6 != 5)) {
    	return 1;
    }

    while ( divisor <= (number / 2) ) {
        if (number % divisor == 0) {
            return 0;
        }
        divisor += 2;
    }
//    append(primes, (ull)number);
    return 1;
}


static void init() {
	if(periodic_perf)
		common_init();

	init_list(primes);

	if(is_tuning_enabled) {
		tuning_library_init();
		tuning_library_start();
	}
}

static inline int __bench_cpu_prime() {
	current_number = 3;

//	append(primes, (ull)2);

	while(1) {
		is_prime(current_number);

		if(unlikely(current_number + 2 >= end_number)) {
			alarm_handler(SIGALRM);
			break;
		}

		if(unlikely(current_number % 9999 == 0))
			nanosleep(&sleep_interval, NULL);

		current_number += 2;
	}
	return 0;
}

static inline int __bench_cpu() {
	current_number = 0;
	while(1) {
		current_number++;
		if(unlikely(current_number % 10000 == 0))
			nanosleep(&sleep_interval, NULL);
		if(unlikely(current_number == end_number))
			alarm_handler(SIGALRM);
	}
	return 0;
}

static int run_bench_cpu(int argc, char **argv) {
	parse_opts(argc, argv);
	init();

	signal(SIGALRM, alarm_handler);
	if(periodic_perf) {
		signal(SIGUSR1, perf_handler);
		perf_reset_stats();
	}
	setitimer(ITIMER_REAL, &timeout_timer, 0);
	start_time = rdclock();
	__bench_cpu();

	return 0;
}


struct benchmark cpu = {
	.usage				= usage,
	.parse_opts			= parse_opts,
	.alarm_handler		= alarm_handler,
	.gracefully_exit	= gracefully_exit,
	.run				= run_bench_cpu,
};
