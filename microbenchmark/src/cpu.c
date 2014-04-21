#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

unsigned int end_number = ~0, current_number = 0, finish_number = -1;
uint64_t start_time, end_time, finish_time;
struct itimerval timeout_timer;
bool done = false;

static int gracefully_exit() {
	printf("Time elapsed  :%0.6fs\n", ((finish_time - start_time) / 1e9) );
	printf("Last number   :%u\n", finish_number);

	done=true;
	return 0;
	//exit(0);
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
    return 1;
}

void run_bench_cpu(float time_sec) {

	current_number = 3;
	done = false;
	end_time = (uint64_t)(time_sec * 1e9);
	time_t sec 	= end_time / 1e9;
	time_t usec = ( (end_time - (sec * 1e9)) / 1e3);
	timeout_timer.it_interval.tv_usec = 0;
	timeout_timer.it_interval.tv_sec  = 0;
	timeout_timer.it_value.tv_sec     = sec;
	timeout_timer.it_value.tv_usec     = usec;
	
	signal(SIGALRM, alarm_handler);

	start_time = rdclock();
	setitimer(ITIMER_REAL, &timeout_timer, 0);
	
	//starting loop
	while(!done) {
		is_prime(current_number);

		if(unlikely(current_number + 2 >= end_number)) {
			alarm_handler(SIGALRM);
			break;
		}
		current_number += 2;
	}
}

