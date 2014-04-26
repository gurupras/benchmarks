/*
 * tuning_library.c
 *
 *  Created on: Apr 19, 2014
 *      Author: guru
 */

#include "tuning_library.h"
#include "power_model_cpu.h"
#include "power_model_mem.h"

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

#define POWER_AGILE_INEFFICIENCY_BUDGET 	"power_agile_task_inefficiency_budget"
#define POWER_AGILE_CONTROLLER				"power_agile_controller"
#define POWER_AGILE_TASK_STATS				"power_agile_task_stats"

#define DIFF_STATS(stat, prev_stat, field) (stat.field - prev_stat.field)

enum COMPONENT {
	CPU,
	MEM,
	NET,
	NR_COMPONENTS,
};

struct component_inefficiency {
	int values[NR_COMPONENTS];
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
	u64 cpu_emin;
};

struct mem_stats {
	u64 mem_busy_time;
	u64 mem_idle_time;
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
	u64 mem_emin;
	u32 mem_freq;
};

struct net_stats {
	u32 net_inefficiency;
	u64 net_emin;
};

struct stats {
	struct base_stats base;
	struct cpu_stats cpu;
	struct mem_stats mem;
	struct net_stats net;
	u64 cur_time;
};

static char *inefficiency_budget_path;
static char *controller_path;
static char *task_stats_path;

static struct stats *prev_stats = NULL;
static unsigned int is_tuning_disabled = 0;
static unsigned int interval = 100 * 1000;	//in us
static pid_t my_pid = -1;

static int is_cpu_tunable = 0, is_mem_tunable = 0, is_net_tunable = 0;
static unsigned int cpu_max_inefficiency, mem_max_inefficiency, net_max_inefficiency;
static const char *components_str[] = {"cpu", "mem", "net"};


static inline u64 get_process_time(void) {
	struct timespec ts;

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
	return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

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
	map->values[CPU] = atoi(ptr);

	ptr = strsep(&tmp, " ");
	map->values[MEM] = atoi(ptr);

	ptr = strsep(&tmp, " ");
	map->values[NET] = atoi(ptr);

	printf("Controller MAX inefficiencies  :%d %d %d\n", map->values[CPU], map->values[MEM], map->values[NET]);
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

	sprintf(buf, "%d", map->values[CPU]);
	sprintf(buf, "%s %d", buf, map->values[MEM]);
	sprintf(buf, "%s %d", buf, map->values[NET]);
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

//	6

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

		str = strsep(&ptr, " ");
		stats->cpu.cpu_emin						= strtoull(str, NULL, 0);
	}

//	13


//	MEM STATS
	if(is_mem_tunable) {
		str = strsep(&ptr, " ");
		stats->mem.mem_busy_time				= strtoull(str, NULL, 0);

		str = strsep(&ptr, " ");
		stats->mem.mem_idle_time				= strtoull(str, NULL, 0);

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

		str = strsep(&ptr, " ");
		stats->mem.mem_emin						= strtoull(str, NULL, 0);

		str = strsep(&ptr, " ");
		stats->mem.mem_freq						= strtoull(str, NULL, 0);
	}

//	27

//	NET STATS
	if(is_net_tunable) {
		str = strsep(&ptr, " ");
		stats->net.net_inefficiency				= strtoull(str, NULL, 0);

		str = strsep(&ptr, " ");
		stats->net.net_emin						= strtoull(str, NULL, 0);
	}

//	29

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
	int fd = open(inefficiency_budget_path, O_RDONLY);
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

static int write_inefficiency_budget(int budget) {
	int err = 0;
	char buf[64];
	int fd = open(inefficiency_budget_path, O_WRONLY);
	if(fd < 0) {
		perror("Could not open inefficiency_budget\n");
		return fd;
	}
	bzero(buf, sizeof buf);
	sprintf(buf, "%d\n", budget);
	err = write(fd, buf, strlen(buf));
	if(err < 0) {
		perror("Unable to write inefficiency_budget\n");
		return err;
	}
	close(fd);

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

static void compute_inefficiency_targets(struct stats *stats, struct stats *prev, struct component_inefficiency *component_inefficiency) {
	u64 cur_time		= get_process_time();
	u64 quantum_time	= cur_time - prev_stats->cur_time;
	stats->cur_time		= cur_time;

	u64 cpu_idle_time	= DIFF_STATS(stats->cpu, prev_stats->cpu, cpu_idle_time);
	u64 cpu_busy_time	= quantum_time - cpu_idle_time;

	u64 mem_busy_time	= DIFF_STATS(stats->mem, prev_stats->mem, mem_busy_time);

	u64 freq, volt, cpu_energy, mem_energy;
	u64 target_cpu_energy, target_mem_energy, target_cpu_frequency, target_mem_frequency;
	int inefficiency_budget;
	read_inefficiency_budget(&inefficiency_budget);

	u64 cpu_emin		= compute_cpu_emin(DIFF_STATS(stats->cpu, prev_stats->cpu, cpu_busy_cycles), DIFF_STATS(stats->cpu, prev_stats->cpu, cpu_idle_time));
	u64 mem_emin		= compute_mem_emin(
			DIFF_STATS(stats->mem, prev_stats->mem, mem_actpreread_events),
			DIFF_STATS(stats->mem, prev_stats->mem, mem_actprewrite_events),
			DIFF_STATS(stats->mem, prev_stats->mem, mem_reads),
			DIFF_STATS(stats->mem, prev_stats->mem, mem_writes),
			DIFF_STATS(stats->mem, prev_stats->mem, mem_refresh_events),
			DIFF_STATS(stats->mem, prev_stats->mem, mem_active_time),
			DIFF_STATS(stats->mem, prev_stats->mem, mem_precharge_time),
			stats->mem.mem_freq
			);
	u64 net_emin  = 1;
	u64 total_budget = (cpu_emin + mem_emin + stats->net.net_emin) * (u64) inefficiency_budget;

	printf("Emins : %llu %llu %llu\n", cpu_emin, mem_emin, stats->net.net_emin);
	printf("Total budget               :%llu\n", total_budget);
	printf("Quantum time(clock)        :%llu\n", quantum_time);
	printf("Total time(diff)           :%llu\n", DIFF_STATS(stats->cpu, prev_stats->cpu, cpu_total_time));

	double cpu_load = ((double) cpu_busy_time / (double) quantum_time);
	printf("CPU load                   :%f\n", cpu_load);


	component_inefficiency->values[CPU] = cpu_load * cpu_max_inefficiency;
	component_inefficiency->values[CPU] = component_inefficiency->values[CPU] < 1000 ? 1000 : component_inefficiency->values[CPU];
	component_inefficiency->values[CPU] = component_inefficiency->values[CPU] > cpu_max_inefficiency ? cpu_max_inefficiency : component_inefficiency->values[CPU];
	printf("CPU inefficiency           :%d\n", component_inefficiency->values[CPU]);
	double mem_load = ((double) mem_busy_time / (double) quantum_time);
	printf("MEM load                   :%f\n", mem_load);
	component_inefficiency->values[MEM] = mem_load * mem_max_inefficiency;
	component_inefficiency->values[MEM] = component_inefficiency->values[MEM] < 1000 ? 1000 : component_inefficiency->values[MEM];
	component_inefficiency->values[MEM] = component_inefficiency->values[MEM] > cpu_max_inefficiency ? cpu_max_inefficiency : component_inefficiency->values[MEM];
	printf("MEM inefficiency           :%d\n", component_inefficiency->values[MEM]);
	component_inefficiency->values[NET] = 1000;

	while(1) {
		int lhs = (component_inefficiency->values[CPU] * cpu_emin) +
				  (component_inefficiency->values[MEM] * mem_emin) +
				  (component_inefficiency->values[NET] * net_emin);

		if(lhs > total_budget) {
			component_inefficiency->values[CPU] -= 100;
			if(component_inefficiency->values[CPU] < 1000)
				component_inefficiency->values[CPU] = 1000;

			component_inefficiency->values[MEM] -= 100;
			if(component_inefficiency->values[MEM] < 1000)
				component_inefficiency->values[MEM] = 1000;
		}
		else if(lhs <= total_budget)
			break;

		if(component_inefficiency->values[CPU] == 1000 && component_inefficiency->values[MEM] == 1000 && component_inefficiency->values[NET] == 1000)
			break;
	}

	// Best frequency matching target cpu inefficiency
	target_cpu_energy = component_inefficiency->values[CPU] * cpu_emin;
	for(freq=CPUmaxFreq, volt=CPUmaxVolt; freq >=CPUminFreq; freq-=CPUfStep, volt-=CPUVStep ) {
		cpu_energy = compute_cpu_energy (DIFF_STATS(stats->cpu, prev_stats->cpu, cpu_busy_cycles), DIFF_STATS(stats->cpu, prev_stats->cpu, cpu_idle_time), freq, volt);
		if(cpu_energy < target_cpu_energy)
			break;
	}
	if(freq >= CPUminFreq)
		target_cpu_frequency = freq;
	else
		target_cpu_frequency = CPUminFreq;

	//Best frequency matching target mem inefficiency
	target_mem_energy = component_inefficiency->values[MEM] * cpu_emin;
	for(freq=maxMemFreq; freq >=minMemFreq; freq -=memfStep) {
		mem_energy = compute_mem_energy(
			DIFF_STATS(stats->mem, prev_stats->mem, mem_actpreread_events),
			DIFF_STATS(stats->mem, prev_stats->mem, mem_actprewrite_events),
			DIFF_STATS(stats->mem, prev_stats->mem, mem_reads),
			DIFF_STATS(stats->mem, prev_stats->mem, mem_writes),
			DIFF_STATS(stats->mem, prev_stats->mem, mem_refresh_events),
			DIFF_STATS(stats->mem, prev_stats->mem, mem_active_time),
			DIFF_STATS(stats->mem, prev_stats->mem, mem_precharge_time),
			freq);
		if(mem_energy < target_mem_energy)
			break;
	}
	if(freq >= minMemFreq)
		target_mem_frequency = freq;
	else
		target_mem_frequency = minMemFreq;
}

static void run_tuning_algorithm(int signal) {
	int err = 0;
	if(prev_stats == NULL) {
		int inefficiency_budget;
		err = read_inefficiency_budget(&inefficiency_budget);
		if(err)
			return;

		struct component_inefficiency component_inefficiency;

		component_inefficiency.values[CPU]	= inefficiency_budget;
		component_inefficiency.values[MEM]	= inefficiency_budget;
		component_inefficiency.values[NET]	= inefficiency_budget;
		write_controller(&component_inefficiency);
		prev_stats = malloc(sizeof(struct stats));
		read_stats(prev_stats);
	}
	else {
		struct stats *stats;
		stats = malloc(sizeof(struct stats));
		read_stats(stats);
		struct component_inefficiency component_inefficiency;
		compute_inefficiency_targets(stats, prev_stats, &component_inefficiency);

		write_controller(&component_inefficiency);
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
	inefficiency_budget_path = path;

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

void tuning_library_set_budget(int val) {
	write_inefficiency_budget(val);
}
