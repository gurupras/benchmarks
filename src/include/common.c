/*
 * common.c
 *
 *  Created on: May 31, 2013
 *      Author: guru
 */

#include "common.h"
#include <fcntl.h>

ull start_time, end_time, finish_time = 0;
ull end_number = ~0, current_number = 0, finish_number = -1;

unsigned int repeat_count, repeat_index;
unsigned int periodic_perf;

struct timespec *sleep_interval;

struct itimerval timeout_timer;

char *error_msg;


inline void panic() {
	fprintf(stderr, "%s\n", error_msg);
	exit(-1);
}

inline unsigned long long rdclock(void) {
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
 }

void periodic_perf_handler(int signal) {
	int pid = getpid();
	char proc_path[32];
	char cycles_path[64];
	char instr_path[64];
	char cache_miss_path[64];

	snprintf(proc_path, 32, "/proc/%d", pid);
	snprintf(cycles_path, 64, "%s/periodic_perf_cycles", proc_path);
	snprintf(instr_path, 64, "%s/periodic_perf_instr", proc_path);
	snprintf(cache_miss_path, 64, "%s/periodic_perf_cache_miss", proc_path);

	char cycles[32], instr[32], cache_miss[32];
	int fd, ret;
	fd = open(cycles_path, O_RDONLY);
	if(fd < 0)
		perror("Could not open periodic_perf_cycles\n");
	ret = read(fd, cycles, sizeof cycles);
	if(ret < 0)
		perror("Could not read periodic_perf_cycles\n");
	close(fd);

	fd = open(instr_path, O_RDONLY);
	if(fd < 0)
		perror("Could not open periodic_perf_instr\n");
	ret = read(fd, instr, sizeof instr);
	if(ret < 0)
		perror("Could not read periodic_perf_instr\n");
	close(fd);

	fd = open(cache_miss_path, O_RDONLY);
	if(fd < 0)
		perror("Could not open periodic_perf_cache_miss\n");
	ret = read(fd, cache_miss, sizeof cache_miss);
	if(ret < 0)
		perror("Could not read periodic_perf_cache_miss\n");
	close(fd);

	printf("Cycles       :%s\n", cycles);
	printf("Instructions :%s\n", instr);
	printf("Cache-misses :%s\n", cache_miss);
}

void reset_periodic_perf_stats() {
	int pid = getpid();
	char proc_path[32];
	char cycles_path[64];
	char instr_path[64];
	char cache_miss_path[64];

	snprintf(proc_path, 32, "/proc/%d", pid);
	snprintf(cycles_path, 64, "%s/periodic_perf_cycles", proc_path);
	snprintf(instr_path, 64, "%s/periodic_perf_instr", proc_path);
	snprintf(cache_miss_path, 64, "%s/periodic_perf_cache_miss", proc_path);

	char *value = "0x0";
	char cycles[32], instr[32], cache_miss[32];
	int fd, ret;
	fd = open(cycles_path, O_WRONLY);
	if(fd < 0)
		perror("Could not open periodic_perf_cycles\n");
	ret = write(fd, value, strlen(value));
	if(ret < 0)
		perror("Could not write periodic_perf_cycles\n");
	close(fd);

	fd = open(instr_path, O_WRONLY);
	if(fd < 0)
		perror("Could not open periodic_perf_instr\n");
	ret = write(fd, value, strlen(value));
	if(ret < 0)
		perror("Could not write periodic_perf_instr\n");
	close(fd);

	fd = open(cache_miss_path, O_WRONLY);
	if(fd < 0)
		perror("Could not open periodic_perf_cache_miss\n");
	ret = write(fd, value, strlen(value));
	if(ret < 0)
		perror("Could not write periodic_perf_cache_miss\n");
	close(fd);
}

int bench_io(int argc, char **argv){}
