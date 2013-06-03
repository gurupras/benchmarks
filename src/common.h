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


typedef unsigned long long ull;

extern ull start_time, end_time;

extern struct itimerval timeout_timer;

extern char *error_msg;

int bench_cpu(int argc, char **argv);
int bench_mem(int argc, char **argv);
int bench_io(int argc, char **argv);

inline void panic();
inline ull rdclock(void);

#endif /* COMMON_H_ */
