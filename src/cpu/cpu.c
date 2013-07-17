/*
 * cpu.c
 *
 *  Created on: May 31, 2013
 *      Author: guru
 */

#include <include/common.h>

static unsigned int sleep_interval, repeat_count, repeat_index;

static ull *runs_end_numbers, *runs_end_times;
static ull primes_size;

static void usage(char *error) {
	struct _IO_FILE *file = stdout;

	if(error != " ")
		file = stderr;
	fprintf(file, "%s\n"
			"bench cpu <option>\n"
			"Benchmarks the CPU by running a primality test until conditions specified have been satisfied\n"
			"\nOPTIONS:\n"
			"    -h        : Print this help message\n"
			"    -t <n>    : Specifies a time limit (in micro seconds)\n"
			"    -n <n>    : Specifies a number limit\n"
			"    -s <n>    : Add an idle duration for every 1000 numbers tested\n"
			"    -r <n>    : Repeat benchmark and print average   (max:100)\n"
			"\nNOTE:\n"
			"At least one of -t or -n must be set. \n"
			"If unset, the program terminates as there is no break condition\n"
			"If both are set, the program terminates upon satisfying either condition\n"
			, error);

	if(error != " ")
		exit(-1);
}

static int parse_opts(int argc, char **argv) {
	int opt;

	while( (opt = getopt(argc, argv, "t:n:s:r:h")) != -1) {
		switch(opt) {
		case ':' :
			usage("missing parameter value");
			break;
		case '?' :
			usage("invalid command line argument");
			break;
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
			end_number = strtoull(optarg, 0, 0);
			break;
		case 's' :
			sleep_interval = atol(optarg);
			break;
		case 'r' :
			repeat_count = atoi(optarg);
			break;
		case 'h' :
			usage(" ");
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
	if(repeat_count) {
		append(runs_end_numbers, finish_number);
		append(runs_end_times, ((finish_time - start_time) / 1e9) );

		if(repeat_index == repeat_count) {
			ull avg_time = 0, avg_num = 0;
			int count = 0;
			ull *entry;
			for_each_entry(entry, runs_end_numbers) {
				avg_num	   += *entry;
				count++;
			}
			for_each_entry(entry, runs_end_times)
				avg_time   += *entry;

			printf("Average time elapsed  :%0.6fs\n", ((avg_time / count) / 1e9) );
			printf("Average last number   :%llu\n", (avg_num / count) );
			exit(0);
		}
	}

	else {
		printf("Time elapsed  :%0.6fs\n", ((finish_time - start_time) / 1e9) );
		printf("Last number   :%llu\n", finish_number);
		exit(0);
	}
}

static void alarm_handler(int signal) {
	finish_time = rdclock();
	finish_number = current_number;

	if(repeat_count) {
		append(runs_end_numbers, finish_number);
		append(runs_end_times, ((finish_time - start_time) / 1e9) );
	}
	gracefully_exit();
}


static ull *primes;

static inline int is_prime(int number) {

    ull divisor = 3;

    if(number == 1ULL ) {
        //1 is neither prime nor composite
        return 0;
    }

    if(number % 2 == 0) {
        return 0; //even no.
    }

//    Every prime number is of the form 6n (+|-) 1
    if(number % 6 != 1 || number % 6 != 5) {
    	return 1;
    }

    while ( divisor <= (number / 2) ) {
        if (number % divisor == 0) {
            return 0;
        }
        divisor += 2;
    }
    append(primes, number);
    return 1;
}


static void init() {
	if(repeat_count) {
		runs_end_numbers	= malloc(sizeof(ull) * repeat_count);
		runs_end_times		= malloc(sizeof(ull) * repeat_count);
	}

	/*
	 * We use the folowing list to maintain a list of computed primes.
	 * These primes help in faster primality testing.
	 * Think of it like the Sieve of Eratosthenes
	 */
	if(end_number != ~0)
		primes_size = sizeof(ull) * (end_number / 2);
	else
		primes_size = sizeof(ull) * 10 * 1024;	//Allocate 10K

	primes = malloc(primes_size);
}

static inline int __bench_cpu() {
	current_number = 0;

	bzero(primes, primes_size);

	append(primes, 2);
	signal(SIGALRM, alarm_handler);


	setitimer(ITIMER_REAL, &timeout_timer, 0);
	start_time = rdclock();

	while(1) {
		is_prime(current_number);

		if(current_number % 1000 == 0 && current_number > 0)
			usleep(sleep_interval);

		if(current_number == end_number)
			break;

		current_number++;
	}

	return 0;
}

static int run_bench_cpu(int argc, char **argv) {
	parse_opts(argc, argv);
	init();

	if(repeat_count) {
		for(repeat_index = 0; repeat_index < repeat_count; repeat_index++) {
			__bench_cpu();
		}
	}
	else {
		__bench_cpu();
	}
	alarm_handler(SIGALRM);
}


struct benchmark cpu = {
	.usage				= usage,
	.parse_opts			= parse_opts,
	.alarm_handler		= alarm_handler,
	.gracefully_exit	= gracefully_exit,
	.run				= run_bench_cpu,
};
