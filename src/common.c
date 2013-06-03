/*
 * common.c
 *
 *  Created on: May 31, 2013
 *      Author: guru
 */

#include "common.h"

ull start_time = 0, end_time = ~0;

struct itimerval timeout_timer;

char *error_msg;


inline void panic() {
	printf("%s\n", error_msg);
	exit(-1);
}

inline unsigned long long rdclock(void) {
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
 }

int bench_mem(int argc, char **argv){}
int bench_io(int argc, char **argv){}
