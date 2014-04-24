/*
 * tuning_library.c
 *
 *  Created on: Apr 19, 2014
 *      Author: guru
 */

#include "tuning_library.h"

#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

typedef unsigned long long u64;
typedef unsigned int u32;

#define POWER_AGILE_INEFFICIENCY_BUDGET 	"power_agile_task_inefficiency_budget"
#define POWER_AGILE_CONTROLLER				"power_agile_controller"
#define POWER_AGILE_TASK_STATS				"power_agile_task_stats"


static int is_cpu_tunable = 0, is_mem_tunable = 0, is_net_tunable = 0;
static unsigned int cpu_max_inefficiency, mem_max_inefficiency, net_max_inefficiency;

enum COMPONENT {
	CPU,
	MEM,
	NET,
	NR_COMPONENTS,
};

static const char *components_str[] = {"cpu", "mem", "net"};

struct component_inefficiency {
	enum COMPONENT component;
	unsigned int inefficiency;
};

struct base_stats {
	u64 insts, cycles;
	u64 user_insts, kernel_insts;
	u64 user_cycles, kernel_cycles;
};

struct cpu_stats {
	u64 cpu_busy_cycles;
	u64 cpu_idle_time;
	u64 cpu_total_time;
	u32 cpu_inefficiency;
	u32 cpu_achieved_inefficiency;
	u32 cpu_max_inefficiency;
};

struct mem_stats {
	u64 mem_actpreread_events;
	u64 mem_actprewrite_events;
	u64 mem_reads;
	u64 mem_writes;
	u64 mem_precharge_time;
	u64 mem_active_time;
	u64 mem_refresh_events;
	u32 mem_inefficiency;
	u32 mem_achieved_inefficiency;
	u32 mem_max_inefficiency;
};

struct net_stats {
	u32 net_inefficiency;
};

struct stats {
	struct base_stats base;
	struct cpu_stats cpu;
	struct mem_stats mem;
	struct net_stats net;
};

static char *inefficiency_path;
static char *controller_path;
static char *task_stats_path;

static struct stats *prev_stats;
static unsigned int is_tuning_disabled = 0;
static unsigned int interval = 100;	//in us
static pid_t my_pid = -1;

void tuning_library_set_interval(unsigned int val) {
	interval = val;
}

static int read_controller(struct component_inefficiency *map) {
	int err = 0;
	int fd = open(controller_path, O_RDONLY);
	if(fd < 0) {
		perror("Unable to open controller\n");
		return fd;
	}

	char buf[64];
	char *tmp = buf;
	err = read(fd, buf, sizeof buf);
	if(err < 0) {
		perror("Unable to read controller\n");
		return err;
	}
	close(fd);

	char *ptr = strsep(&tmp, " ");
	map[0].component = CPU;
	map[0].inefficiency = atoi(ptr);

	ptr = strsep(&tmp, " ");
	map[1].component = MEM;
	map[1].inefficiency = atoi(ptr);

	ptr = strsep(&tmp, " ");
	map[2].component = NET;
	map[2].inefficiency = atoi(ptr);

	return 0;
}

static int write_controller(struct component_inefficiency *map) {
	int err = 0;
	int fd = open(controller_path, O_WRONLY);
	if(fd < 0) {
		perror("Unable to open controller\n");
		return fd;
	}
	char buf[32];
	bzero(buf, sizeof buf);

	sprintf(buf, "%d", map[0].inefficiency);
	sprintf(buf, "%s %d", buf, map[1].inefficiency);
	sprintf(buf, "%s %d", buf, map[2].inefficiency);
	sprintf(buf, "%s\n", buf);

	err = write(fd, buf, strlen(buf));
	if(err < 0) {
		perror("Unable to write controller\n");
		return err;
	}
	close(fd);
	return 0;
}

static int read_stats(struct stats *stats) {
	int err = 0;
	int fd = open(task_stats_path, O_RDONLY);
	if(fd < 0) {
		perror("Could not open task_stats\n");
		return fd;
	}
	char buf[1024];
	err = read(fd, buf, sizeof(buf));
	if(err < 0) {
		perror("Could not read task_stats\n");
		return err;
	}

	char *ptr = buf;
	char *str;

//	BASE STATS
	str = strsep(&ptr, " ");
	stats->base.cycles						= strtoull(str, NULL, 0);

	str = strsep(&ptr, " ");
	stats->base.insts						= strtoull(str, NULL, 0);

	str = strsep(&ptr, " ");
	stats->base.user_insts					= strtoull(str, NULL, 0);

	str = strsep(&ptr, " ");
	stats->base.kernel_insts				= strtoull(str, NULL, 0);

	str = strsep(&ptr, " ");
	stats->base.user_cycles					= strtoull(str, NULL, 0);

	str = strsep(&ptr, " ");
	stats->base.kernel_cycles				= strtoull(str, NULL, 0);


//	CPU STATS
	if(is_cpu_tunable) {
		str = strsep(&ptr, " ");
		stats->cpu.cpu_busy_cycles				= strtoull(str, NULL, 0);

		str = strsep(&ptr, " ");
		stats->cpu.cpu_idle_time				= strtoull(str, NULL, 0);

		str = strsep(&ptr, " ");
		stats->cpu.cpu_total_time				= strtoull(str, NULL, 0);

		str = strsep(&ptr, " ");
		stats->cpu.cpu_inefficiency				= strtoull(str, NULL, 0);

		str = strsep(&ptr, " ");
		stats->cpu.cpu_achieved_inefficiency	= strtoull(str, NULL, 0);

		str = strsep(&ptr, " ");
		stats->cpu.cpu_max_inefficiency			= strtoull(str, NULL, 0);
	}


//	MEM STATS
	if(is_mem_tunable) {
		str = strsep(&ptr, " ");
		stats->mem.mem_actpreread_events		= strtoull(str, NULL, 0);

		str = strsep(&ptr, " ");
		stats->mem.mem_actprewrite_events		= strtoull(str, NULL, 0);

		str = strsep(&ptr, " ");
		stats->mem.mem_reads					= strtoull(str, NULL, 0);

		str = strsep(&ptr, " ");
		stats->mem.mem_writes					= strtoull(str, NULL, 0);

		str = strsep(&ptr, " ");
		stats->mem.mem_precharge_time			= strtoull(str, NULL, 0);

		str = strsep(&ptr, " ");
		stats->mem.mem_active_time				= strtoull(str, NULL, 0);

		str = strsep(&ptr, " ");
		stats->mem.mem_refresh_events			= strtoull(str, NULL, 0);

		str = strsep(&ptr, " ");
		stats->mem.mem_inefficiency				= strtoull(str, NULL, 0);

		str = strsep(&ptr, " ");
		stats->mem.mem_achieved_inefficiency	= strtoull(str, NULL, 0);

		str = strsep(&ptr, " ");
		stats->mem.mem_max_inefficiency			= strtoull(str, NULL, 0);
	}

//	NET STATS
	if(is_net_tunable) {
		str = strsep(&ptr, " ");
		stats->net.net_inefficiency				= strtoull(str, NULL, 0);
	}

	return 0;
}

static int write_stats(char *buf) {
	int err = 0;
	int fd = open(task_stats_path, O_WRONLY);
	if(fd < 0) {
		perror("Could not open task_stats\n");
		return fd;
	}

	err = write(fd, buf, strlen(buf));
	if(err < 0) {
		perror("Could not write task_stats\n");
		return err;
	}
	close(fd);
	return 0;
}

static int read_inefficiency_budget(int *budget) {
	int err = 0;
	char buf[64];
	int fd = open(inefficiency_path, O_RDONLY);
	if(fd < 0) {
		perror("Could not open inefficiency_budget\n");
		return fd;
	}
	err = read(fd, buf, sizeof(buf));
	if(err < 0) {
		perror("Unable to read inefficiency_budget\n");
		return err;
	}
	close(fd);

	*budget = atoi(buf);
	return 0;
}

static int read_power_agile_components() {
	char buf[32];
	int err;
	int fd;

	fd = open("/proc/power_agile_inefficiency_components", O_RDONLY);
	if(fd < 0) {
		perror("Unable to open power_agile_components\n");
		return -1;
	}
	err = read(fd, buf, sizeof buf);
	if(err < 0) {
		perror("Unable to read power_agile_components\n");
		return -1;
	}

	char *ptr = buf;
	char *str;

	(void) components_str;
	while((str = strsep(&ptr, " ")) != NULL) {
//		We currently hard-code this
		if(strcmp(str, "cpu") == 0) {
			is_cpu_tunable = 1;
			cpu_max_inefficiency = atoi(strsep(&ptr, " "));
		}
		if(strcmp(str, "mem") == 0) {
			is_mem_tunable = 1;
			mem_max_inefficiency = atoi(strsep(&ptr, " "));
		}
		if(strcmp(str, "net") == 0) {
			is_net_tunable = 1;
			net_max_inefficiency = atoi(strsep(&ptr, " "));
		}
	}
	return 0;
}


static inline void schedule() {
	if(is_tuning_disabled)
		return;

	struct itimerval timer;
	timer.it_interval.tv_sec	= 0;
	timer.it_interval.tv_usec	= 0;
	timer.it_value.tv_sec		= interval / 1e6;
	timer.it_value.tv_usec		= (interval - (timer.it_value.tv_sec * 1e6));
	setitimer(ITIMER_REAL, &timer, NULL);
}



static int compute_cpu_inefficiency_target(struct cpu_stats stats) {
	double busy_percentage = 100 - ((stats.cpu_idle_time / (double) stats.cpu_total_time) * 100);

//	We apply the busy percentage only if it is greater than the up threshold or lower than the down threshold.
//	If this condition was not being checked, then what we would see is a jump to some frequency, then a load of 100%
//	Which would result in a jump to the max frequency and then just alternation between the two states.
		if(busy_percentage >= 80 || busy_percentage <= 20)
			return cpu_max_inefficiency * busy_percentage / 100;
		return stats.cpu_inefficiency;
}

static int compute_mem_inefficiency_target(struct mem_stats stats, u64 total_time) {
	double busy_percentage = 100 - ((stats.mem_active_time / (double) total_time) * 100);
//	We apply the busy percentage only if it is greater than the up threshold or lower than the down threshold.
//	If this condition was not being checked, then what we would see is a jump to some frequency, then a load of 100%
//	Which would result in a jump to the max frequency and then just alternation between the two states.
	if(busy_percentage >= 60 || busy_percentage <= 20)
		return mem_max_inefficiency * busy_percentage / 100;
	return stats.mem_inefficiency;
}

static int compute_net_inefficiency_target(struct net_stats stats) {
	return stats.net_inefficiency;
}

static void run_tuning_algorithm(int signal) {
	int err = 0;
	printf("Running tuning algorithm\n");
	if(prev_stats == NULL) {
		int inefficiency_budget;
		err = read_inefficiency_budget(&inefficiency_budget);
		if(err)
			return;
		struct component_inefficiency component_inefficiency[3];
		component_inefficiency[0].component = CPU;
		component_inefficiency[0].inefficiency	= inefficiency_budget;
		component_inefficiency[1].component = MEM;
		component_inefficiency[1].inefficiency	= inefficiency_budget;
		component_inefficiency[2].component = NET;
		component_inefficiency[3].inefficiency	= inefficiency_budget;
		write_controller(component_inefficiency);
		prev_stats = malloc(sizeof(struct stats));
		read_stats(prev_stats);
	}
	else {
		struct stats *stats;
		stats = malloc(sizeof(struct stats));
		read_stats(stats);
		struct component_inefficiency component_inefficiency[3];
		component_inefficiency[0].inefficiency = compute_cpu_inefficiency_target(stats->cpu);
		component_inefficiency[1].inefficiency = compute_mem_inefficiency_target(stats->mem, stats->cpu.cpu_total_time);
		component_inefficiency[2].inefficiency = compute_net_inefficiency_target(stats->net);
		write_controller(component_inefficiency);
		free(prev_stats);
		prev_stats = stats;
	}
	schedule();
}

int tuning_library_init() {
	my_pid = getpid();
	if(my_pid < 0) {
		return -EINVAL;
	}

	signal(SIGALRM, run_tuning_algorithm);

	char *path = malloc(sizeof(char) * 64);
	bzero(path, 64);
	snprintf(path, 64, "/proc/%d/" POWER_AGILE_INEFFICIENCY_BUDGET, my_pid);
	inefficiency_path = path;

	path = malloc(sizeof(char) * 64);
	bzero(path, 64);
	snprintf(path, 64, "/proc/%d/" POWER_AGILE_CONTROLLER, my_pid);
	controller_path = path;

	path = malloc(sizeof(char) * 64);
	bzero(path, 64);
	snprintf(path, 64, "/proc/%d/" POWER_AGILE_TASK_STATS, my_pid);
	task_stats_path = path;

	read_power_agile_components();

	return 0;
}

void tuning_library_start() {
	is_tuning_disabled = 0;
	run_tuning_algorithm(0);
}

void tuning_library_stop() {
	is_tuning_disabled = 1;
}
