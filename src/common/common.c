/*
 * common.c
 *
 *  Created on: May 31, 2013
 *      Author: guru
 */

#include "include/common.h"
#include "include/perf.h"
#include <fcntl.h>

int verbose = 0;

u64 start_time, end_time, finish_time = 0;

struct itimerval timeout_timer;

char *error_msg;

inline void panic() {
	fprintf(stderr, "%s\n", error_msg);
	exit(-1);
}

inline u64 rdclock(void) {
	struct timespec ts;

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
	return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

void common_init() {
	perf_init();
}


int bench_io(int argc, char **argv) {
	return 0;
}
