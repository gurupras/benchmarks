/*
 * cpu.c
 *
 *  Created on: May 31, 2013
 *      Author: guru
 */

#include <include/common.h>

static void usage(int error) {
	int file = stdout;

	if(error != 0)
		file = stderr;
	fprintf(file, "bench cpu <option>\n"
			"Benchmarks the CPU by running a primality test until conditions specified have been satisfied\n"
			"\nOPTIONS:\n"
			"    -h    : Print this help message\n"
			"    -t    : Specifies a time limit (in seconds)\n"
			"    -n    : Specifies a number limit\n"
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
		case 'h' :
			usage(0);
			break;
		default :
			usage(opt);
			break;
		}
	}
	return 0;
}

static int gracefully_exit() {
	printf("Time elapsed  :%0.6fs\n", ((finish_time - start_time) / 1e9) );
	printf("Last number   :%llu\n", finish_number);
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



static int bench_cpu(int argc, char **argv) {
	ull noop_index = 0;

	parse_opts(argc, argv);

	if(end_time == ~0 && end_number == ~0) {
		bzero(error_msg, strlen(error_msg));
		sprintf(error_msg, "No options specified\nTerminating program due to infinite loop!\n");
		panic();
	}

	ull malloc_size;
	if(end_number != ~0)
		malloc_size = sizeof(ull) * (end_number / 2);
	else
		malloc_size = sizeof(ull) * 10 * 1024;	//Allocate 10K

	primes = malloc(malloc_size);
	bzero(primes, malloc_size);

	append(primes, 2);
	signal(SIGALRM, alarm_handler);


	setitimer(ITIMER_REAL, &timeout_timer, 0);
	start_time = rdclock();

	while(1) {
		is_prime(current_number);
		if(current_number == end_number)
			break;

		current_number++;
	}

	alarm_handler(SIGALRM);
}

struct benchmark cpu = {
	.usage				= usage,
	.parse_opts			= parse_opts,
	.alarm_handler		= alarm_handler,
	.gracefully_exit	= gracefully_exit,
	.run				= bench_cpu,
};
