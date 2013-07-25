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

char **__pos;

#define init_list(list, size) \
{ \
	int i; \
	list = malloc(sizeof(void *) * size); \
	for(i = 0; i < size; i++) \
		list[i] = NULL; \
}

#define append(list,element) \
{ \
	int index = 0; \
	while(list[index] != NULL) \
		index++; \
	typeof(element) *ptr = malloc(sizeof(element)); \
	*ptr = element; \
	list[index] = ptr; \
}

#define for_each_entry(entry, list) \
	for( \
		__pos = (char **)list; \
		__pos != NULL; \
		entry = *((typeof(entry) *)__pos[0]), __pos ++)

typedef unsigned long long ull;

extern ull start_time, end_time, finish_time;
extern ull end_number, current_number, finish_number;


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

#endif /* COMMON_H_ */
