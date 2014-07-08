/*
 * cpu.c
 *
 *  Created on: May 31, 2013
 *      Author: guru
 */

#include "include/common.h"
#include "include/perf.h"
#include "tuning_library.h"

u64 end_number = ~0, current_number = 0, finish_number = -1;
static int budget;
static volatile int is_finished = 0;
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
//			"    -t <n>    : Specifies a time limit (in nanoseconds)\n"
			"    -s <n>    : Specifies an intermittent sleep interval\n"
			"    -n <n>    : Specifies a number limit\n"
			"    -p        : Enable performance counter accounting\n"
			"    -u        : Enable the power-agile tuning library\n"
			"    -b        : Assign inefficiency budget\n"
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

	while( (opt = getopt(argc, argv, "t:n:s:r:b:hpuv")) != -1) {
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
			is_tuning_disabled = 0;
			break;
		case 'b' :
			budget = atoi(optarg);
			break;
		case 'v' :
			verbose = 1;
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

	if(!is_tuning_disabled && budget == 0) {
		usage("Must specify budget!\n");
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
	is_finished = 1;
	return 0;
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
	signal(SIGALRM, alarm_handler);
	if(periodic_perf) {
		common_init();
		signal(SIGUSR1, perf_handler);
		perf_reset_stats();
	}

	init_list(primes);

	if(!is_tuning_disabled) {
		tuning_library_init();
		tuning_library_set_budget(budget);
		tuning_library_start();
	}
}

static inline int __bench_cpu_prime() {
	current_number = 3;

//	append(primes, (ull)2);

	while(!is_finished) {
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
	while(unlikely(is_finished)) {
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
	is_finished = 0;

	setitimer(ITIMER_REAL, &timeout_timer, 0);
	start_time = rdclock();
	__bench_cpu_prime();

	if(!is_tuning_disabled)
		tuning_library_exit();

	return 0;
}

static void timed_bench_cpu(u64 time) {
	if(time == 0ULL)
		return;
	is_finished = 0;
	current_number = 3;
	end_time = time;
	time_t sec 	= end_time / SEC_TO_NSEC;
	time_t usec = ( (end_time - (sec * SEC_TO_NSEC)) / USEC_TO_NSEC);
	timeout_timer.it_interval.tv_usec = 0;
	timeout_timer.it_interval.tv_sec  = 0;
	timeout_timer.it_value.tv_sec     = sec;
	timeout_timer.it_value.tv_usec    = usec;

	signal(SIGALRM, alarm_handler);

	start_time = rdclock();
	setitimer(ITIMER_PROF, &timeout_timer, 0);

	while(!is_finished) {
		is_prime(current_number);

		if(unlikely(current_number + 2 >= end_number)) {
			break;
		}
		current_number += 2;
	}
}

struct benchmark cpu = {
	.usage				= usage,
	.parse_opts			= parse_opts,
	.alarm_handler		= alarm_handler,
	.gracefully_exit	= gracefully_exit,
	.run				= run_bench_cpu,
	.timed_run			= timed_bench_cpu,
};
