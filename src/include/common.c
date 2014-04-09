/*
 * common.c
 *
 *  Created on: May 31, 2013
 *      Author: guru
 */

#include "common.h"
#include <fcntl.h>

ull start_time, end_time, finish_time = 0;

struct itimerval timeout_timer;

char *error_msg;


inline void panic() {
	fprintf(stderr, "%s\n", error_msg);
	exit(-1);
}

inline unsigned long long rdclock(void) {
	struct timespec ts;

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
	return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

void periodic_perf_handler(int signal) {
	int pid = getpid();
	char proc_path[48];
	char cycles_path[48];
	char instr_path[48];
	char cache_miss_path[48];
	char power_agile_task_stats_path[48];

	snprintf(proc_path, 48, "/proc/%d", pid);
	snprintf(cycles_path, 48, "%s/periodic_perf_cycles", proc_path);
	snprintf(instr_path, 48, "%s/periodic_perf_instr", proc_path);
	snprintf(cache_miss_path, 48, "%s/periodic_perf_cache_miss", proc_path);
	snprintf(power_agile_task_stats_path, 48, "%s/power_agile_task_stats", proc_path);

	char cycles[32], instr[32], cache_miss[32], power_agile_task_stats[128];
	int fd, ret;
	fd = open(cycles_path, O_RDONLY);
	if(fd < 0)
		perror("Could not open periodic_perf_cycles\n");
	else {
		ret = read(fd, cycles, sizeof cycles);
		if(ret < 0)
			perror("Could not read periodic_perf_cycles\n");
		else
			printf("Cycles       :%s\n", cycles);
		close(fd);
	}

	fd = open(instr_path, O_RDONLY);
	if(fd < 0)
		perror("Could not open periodic_perf_instr\n");
	else {
		ret = read(fd, instr, sizeof instr);
		if(ret < 0)
			perror("Could not read periodic_perf_instr\n");
		else
			printf("Instructions :%s\n", instr);
		close(fd);
	}

	fd = open(cache_miss_path, O_RDONLY);
	if(fd < 0)
		perror("Could not open periodic_perf_cache_miss\n");
	else {
		ret = read(fd, cache_miss, sizeof cache_miss);
		if(ret < 0)
			perror("Could not read periodic_perf_cache_miss\n");
		else
			printf("Cache-misses :%s\n", cache_miss);
		close(fd);
	}

	fd = open(power_agile_task_stats_path, O_RDONLY);
	if(fd < 0)
		perror("Could not open power_agile_task_stats\n");
	else {
		ret = read(fd, power_agile_task_stats, sizeof power_agile_task_stats);
		if(ret < 0)
			perror("Could not read power_agile_task_stats\n");
		else
			printf("stats        :%s\n", power_agile_task_stats);
		close(fd);
	}
}

void reset_periodic_perf_stats() {
	int pid = getpid();
	char proc_path[48];
	char cycles_path[48];
	char instr_path[48];
	char cache_miss_path[48];
	char power_agile_task_stats_path[48];

	snprintf(proc_path, 48, "/proc/%d", pid);
	snprintf(cycles_path, 48, "%s/periodic_perf_cycles", proc_path);
	snprintf(instr_path, 48, "%s/periodic_perf_instr", proc_path);
	snprintf(cache_miss_path, 48, "%s/periodic_perf_cache_miss", proc_path);
	snprintf(power_agile_task_stats_path, 48, "%s/power_agile_task_stats", proc_path);

	char *value = "0x0";
	int fd, ret;
	fd = open(cycles_path, O_WRONLY);
	if(fd < 0)
		perror("Could not open periodic_perf_cycles\n");
	else {
		ret = write(fd, value, strlen(value));
		if(ret < 0)
			perror("Could not write periodic_perf_cycles\n");
		close(fd);
	}

	fd = open(instr_path, O_WRONLY);
	if(fd < 0)
		perror("Could not open periodic_perf_instr\n");
	else {
		ret = write(fd, value, strlen(value));
		if(ret < 0)
			perror("Could not write periodic_perf_instr\n");
		close(fd);
	}

	fd = open(cache_miss_path, O_WRONLY);
	if(fd < 0)
		perror("Could not open periodic_perf_cache_miss\n");
	else {
		ret = write(fd, value, strlen(value));
		if(ret < 0)
			perror("Could not write periodic_perf_cache_miss\n");
		close(fd);
	}

	fd = open(power_agile_task_stats_path, O_WRONLY);
	if(fd < 0)
		perror("Could not open power_agile_task_stats\n");
	else {
		ret = write(fd, value, strlen(value));
		if(ret < 0)
			perror("Could not write power_agile_task_stats\n");
		close(fd);
	}
}

int bench_io(int argc, char **argv) {
	return 0;
}
