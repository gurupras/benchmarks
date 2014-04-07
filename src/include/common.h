/*
 * common.h
 *
 *  Created on: May 31, 2013
 *      Author: guru
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "list.h"

typedef unsigned long long ull;

extern ull start_time, end_time, finish_time;
extern ull end_number, current_number, finish_number;

extern unsigned int repeat_count, repeat_index;
extern unsigned int periodic_perf;

extern struct timespec *sleep_interval;
extern struct itimerval timeout_timer;

extern char *error_msg;

struct benchmark {
	void (*usage) (char *error);
	int  (*parse_opts) (int argc, char **argv);
	void (*alarm_handler) (int signal);
	int  (*gracefully_exit) (void);
	int  (*run) (int argc, char **argv);
};

extern struct benchmark cpu, mem, io;


inline void panic();
inline ull rdclock(void);

inline void periodic_perf_handler(int);
inline void reset_periodic_perf_stats(void);
#endif /* COMMON_H_ */
