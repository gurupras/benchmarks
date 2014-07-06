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
#include <inttypes.h>

#define unlikely(x)		__builtin_expect(!!(x), 0)
#define likely(x)		__builtin_expect(!!(x), 1)

#define USEC_TO_NSEC	(1000)
#define MSEC_TO_NSEC	(1000 * USEC_TO_NSEC)
#define SEC_TO_NSEC		(1000 * MSEC_TO_NSEC)

extern int verbose;

#define VERBOSE(...) \
{ \
	if(verbose) \
		printf(__VA_ARGS__); \
}

#include "list.h"

typedef unsigned long long u64;
typedef unsigned int u32;

extern u64 start_time, end_time, finish_time;



extern struct itimerval timeout_timer;

extern char *error_msg;

struct benchmark {
	void (*init) (void);
	void (*usage) (char *error);
	int  (*parse_opts) (int argc, char **argv);
	void (*alarm_handler) (int signal);
	int  (*gracefully_exit) (void);
	int  (*run) (int argc, char **argv);
	/** Time in ns */
	void (*timed_run) (u64 time);
	void (*operation_run) (int operations);
};

extern struct benchmark cpu, mem, io, micro_benchmark, annotation_test_benchmark;


inline void panic();
inline u64 rdclock(void);

void common_init(void);

#endif /* COMMON_H_ */
