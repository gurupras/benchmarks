
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include "include/perf.h"

unsigned int periodic_perf = 0;

static char stats_path[48];

static int perf_open() {
	return open(stats_path, O_RDWR);
}

void perf_read_stats(u64 *cycles, u64 *insts, u64 *cache_miss) {
	char stats[128];
	int fd, ret;
	fd = perf_open();
	if(fd > 0) {
		ret = read(fd, stats, sizeof stats);
		if(ret < 0)
			perror("Could not read perf_stats\n");

		char *ptr 	= strtok(stats, " ");
		if(cycles)
			*cycles 	= strtoull(ptr, NULL, 0);

		ptr 		= strtok(NULL, " ");
		if(insts)
			*insts		= strtoull(ptr, NULL, 0);

		ptr 		= strtok(NULL, " ");
		if(cache_miss)
			*cache_miss	= strtoull(ptr, NULL, 0);
		close(fd);
	}
	else
		perror("Could not open perf_stats\n");
}

void perf_write_stats(char *val) {
	int fd, ret;
	fd = perf_open();
	if(fd > 0) {
		ret = write(fd, val, strlen(val));
		if(ret < 0)
			perror("Could not write perf_stats\n");
		close(fd);
	}
	else
		perror("Could not open perf_stats\n");
}

void perf_handler(int signal) {
	u64 cycles = 0, insts = 0, cache_miss = 0;

	perf_read_stats(&cycles, &insts, &cache_miss);
	printf("Cycles       :%llu\n", cycles);
	printf("Instructions :%llu\n", insts);
	printf("Cache-misses :%llu\n", cache_miss);
}

void perf_init() {
	int fd;
	int pid;
	char proc_path[48];
	char periodic_perf_path[64], power_agile_path[64];

	pid = getpid();

	snprintf(proc_path, 48, "/proc/%d", pid);
	snprintf(power_agile_path, 48, "%s/power_agile_task_stats", proc_path);
	snprintf(periodic_perf_path, 48, "%s/periodic_perf_stats", proc_path);

	fd = open(periodic_perf_path, O_RDWR);
	if(fd > 0) {
		strcpy(stats_path, periodic_perf_path);
		return;
	}
	fd = open(power_agile_path, O_RDWR);
	if(fd > 0) {
		strcpy(stats_path, power_agile_path);
		return;
	}
	else
		panic("perf init failed!");
}

void perf_reset_stats() {
	char *value = "0";
	perf_write_stats(value);
}
