
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include "include/perf.h"

unsigned int periodic_perf = 0;

static char proc_path[48];
static char cycles_path[48];
static char instr_path[48];
static char cache_miss_path[48];
static char power_agile_task_stats_path[48];

static int perf_open_periodic_perf_cycles() {
	return open(cycles_path, O_RDWR);
}

static int perf_open_periodic_perf_insts() {
	return open(instr_path, O_RDWR);
}

static int perf_open_periodic_perf_cache_miss() {
	return open(cache_miss_path, O_RDWR);
}

static int perf_open_power_agile_task_stats() {
	return open(power_agile_task_stats_path, O_RDWR);
}

ull perf_read_periodic_perf_cycles() {
	char cycles[32];
	int fd, ret;
	ull val = 0;
	fd = perf_open_periodic_perf_cycles();
	if(fd > 0) {
		ret = read(fd, cycles, sizeof cycles);
		if(ret < 0)
			perror("Could not read periodic_perf_cycles\n");
		val = strtoull(cycles, NULL, 0);
		close(fd);
	}
	else
		perror("Could not open periodic_perf_cycles\n");
	return val;
}

ull perf_read_periodic_perf_insts() {
	char insts[32];
	int fd, ret;
	ull val = 0;
	fd = perf_open_periodic_perf_insts();
	if(fd > 0) {
		ret = read(fd, insts, sizeof insts);
		if(ret < 0)
			perror("Could not read periodic_perf_cycles\n");
		val = strtoull(insts, NULL, 0);
		close(fd);
	}
	else
		perror("Could not open periodic_perf_insts\n");
	return val;
}

ull perf_read_periodic_perf_cache_miss() {
	char cache_miss[32];
	int fd, ret;
	ull val = 0;
	fd = perf_open_periodic_perf_cache_miss();
	if(fd > 0) {
		ret = read(fd, cache_miss, sizeof cache_miss);
		if(ret < 0)
			perror("Could not read periodic_perf_cache_miss\n");
		val = strtoull(cache_miss, NULL, 0);
		close(fd);
	}
	else
		perror("Could not open periodic_perf_cache_miss\n");
	return val;
}

void perf_read_power_agile_task_stats(char *buf) {
	int fd, ret;
	bzero(buf, 128);
	fd = perf_open_power_agile_task_stats();
	if(fd > 0) {
		ret = read(fd, buf, 128);
		if(ret < 0)
			perror("Could not read power_agile\n");
		close(fd);
	}
	else
		perror("Could not open power_agile_task_stats\n");
}

void perf_write_periodic_perf_cycles(char *val) {
	int fd, ret;
	fd = perf_open_periodic_perf_cycles();
	if(fd > 0) {
		ret = write(fd, val, strlen(val));
		if(ret < 0)
			perror("Could not write periodic_perf_cycles\n");
		close(fd);
	}
	else
		perror("Could not open periodic_perf_cycles\n");
}

void perf_write_periodic_perf_insts(char *val) {
	int fd, ret;
	fd = perf_open_periodic_perf_insts();
	if(fd > 0) {
		ret = write(fd, val, strlen(val));
		if(ret < 0)
			perror("Could not write periodic_perf_insts\n");
		close(fd);
	}
	else
		perror("Could not open periodic_perf_insts\n");
}

void perf_write_periodic_perf_cache_miss(char *val) {
	int fd, ret;
	fd = perf_open_periodic_perf_cache_miss();
	if(fd > 0) {
		ret = write(fd, val, strlen(val));
		if(ret < 0)
			perror("Could not write periodic_perf_cache_miss\n");
		close(fd);
	}
	else
		perror("Could not open periodic_perf_cache_miss\n");
}

void perf_write_power_agile_task_stats(char *val) {
	int fd, ret;
	fd = perf_open_power_agile_task_stats();
	if(fd > 0) {
		ret = write(fd, val, strlen(val));
		if(ret < 0)
			perror("Could not write power_agile_task_stats\n");
		close(fd);
	}
	else
		perror("Could not open power_agile_task_stats\n");
}

void periodic_perf_handler(int signal) {
	char power_agile_task_stats[128];
	ull cycles = 0, insts = 0, cache_miss = 0;

	cycles		= perf_read_periodic_perf_cycles();
	insts		= perf_read_periodic_perf_insts();
	cache_miss	= perf_read_periodic_perf_cache_miss();
	perf_read_power_agile_task_stats(power_agile_task_stats);
	printf("Cycles       :%llu\n", cycles);
	printf("Instructions :%llu\n", insts);
	printf("Cache-misses :%llu\n", cache_miss);
	printf("stats        :%s\n", power_agile_task_stats);
}

void periodic_perf_init() {
	int pid = getpid();

	snprintf(proc_path, 48, "/proc/%d", pid);
	snprintf(cycles_path, 48, "%s/periodic_perf_cycles", proc_path);
	snprintf(instr_path, 48, "%s/periodic_perf_instr", proc_path);
	snprintf(cache_miss_path, 48, "%s/periodic_perf_cache_miss", proc_path);
	snprintf(power_agile_task_stats_path, 48, "%s/power_agile_task_stats", proc_path);
}

void reset_periodic_perf_stats() {
	char *value = "0";
	perf_write_periodic_perf_cycles(value);
	perf_write_periodic_perf_insts(value);
	perf_write_periodic_perf_cache_miss(value);
	perf_write_power_agile_task_stats(value);
}
