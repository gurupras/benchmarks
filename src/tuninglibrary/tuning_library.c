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
#include <stdarg.h>

#define POWER_AGILE_INEFFICIENCY_BUDGET 	"power_agile_task_inefficiency_budget"
#define POWER_AGILE_CONTROLLER				"power_agile_controller"
#define POWER_AGILE_TASK_STATS				"power_agile_task_stats"

#define DIFF_STATS(stat, prev_stat, field) (stat.field - prev_stat.field)

#define LOGSIZE		65536
enum COMPONENT {
	CPU,
	MEM,
	NET,
	NR_COMPONENTS,
};

struct component_settings {
	int frequency[NR_COMPONENTS];
	int inefficiency[NR_COMPONENTS];
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
	u64 cpu_energy;
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
	u64 mem_energy;
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

unsigned int is_tuning_disabled = 1;
static int is_init_complete = 0;

static char *logbuf;
static char *inefficiency_budget_path;
static char *controller_path;
static char *task_stats_path;

static struct stats *prev_stats = NULL;
static unsigned int interval = 100 * 1000;	//in us
static pid_t my_pid = -1;

static int is_cpu_tunable = 0, is_mem_tunable = 0, is_net_tunable = 0;
static unsigned int cpu_max_inefficiency, mem_max_inefficiency, net_max_inefficiency;
static const char *components_str[] = {"cpu", "mem", "net"};

u64 mem_curr_freq, mem_new_freq=800;

static inline void tuning_library_log(const char *fmt, ...) {
	va_list argptr;
	va_start(argptr, fmt);
	vsnprintf(logbuf + strlen(logbuf), LOGSIZE - strlen(logbuf), fmt, argptr);
	va_end(argptr);
}

static inline u64 get_process_time(void) {
	struct timespec ts;

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
	return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

void tuning_library_set_interval(unsigned int val) {
	interval = val;
}

static int read_controller(struct component_settings *map) {
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
	map->inefficiency[CPU] = atoi(ptr);

	ptr = strsep(&tmp, " ");
	map->inefficiency[MEM] = atoi(ptr);

	ptr = strsep(&tmp, " ");
	map->inefficiency[NET] = atoi(ptr);

	tuning_library_log("Controller MAX inefficiencies  :%d %d %d\n", map->inefficiency[CPU], map->inefficiency[MEM], map->inefficiency[NET]);
	return 0;
}

static int write_controller(struct component_settings *map) {
	int err = 0;
	int fd = open(controller_path, O_WRONLY);
	if(fd < 0) {
		perror("Unable to open controller\n");
		return fd;
	}
	char buf[64];
	bzero(buf, sizeof buf);

	sprintf(buf, "%d", map->frequency[CPU]);
	sprintf(buf, "%s %d", buf, map->inefficiency[CPU]);

	sprintf(buf, "%s %d", buf, map->frequency[MEM]);
	sprintf(buf, "%s %d", buf, map->inefficiency[MEM]);

	sprintf(buf, "%s %d", buf, map->frequency[NET]);
	sprintf(buf, "%s %d", buf, map->inefficiency[NET]);

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

		str = strsep(&ptr, " ");
		stats->cpu.cpu_energy					= strtoull(str, NULL, 0);
	}

//	14


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

		str = strsep(&ptr, " ");
		stats->mem.mem_energy					= strtoull(str, NULL, 0);
	}

//	29

//	NET STATS
	if(is_net_tunable) {
		str = strsep(&ptr, " ");
		stats->net.net_inefficiency				= strtoull(str, NULL, 0);

		str = strsep(&ptr, " ");
		stats->net.net_emin						= strtoull(str, NULL, 0);
	}

//	31

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
	tuning_library_log("MAX Inefficiencies :%u %u %u\n", cpu_max_inefficiency, mem_max_inefficiency, net_max_inefficiency);
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

static void compute_inefficiency_targets(struct stats *stats, struct stats *prev, struct component_settings *component_settings) {
	u64 quantum_time;
	u64 cur_time		= get_process_time();
	mem_curr_freq = mem_new_freq;
//	quantum_time		= cur_time - prev_stats->cur_time;
	quantum_time		= stats->cpu.cpu_total_time;
	stats->cur_time		= cur_time;

	u64 cpu_idle_time	= DIFF_STATS(stats->cpu, prev_stats->cpu, cpu_idle_time);
	u64 cpu_busy_time	= quantum_time - cpu_idle_time;

	u64 mem_busy_time	= DIFF_STATS(stats->mem, prev_stats->mem, mem_busy_time);
	u64 mem_reads		= DIFF_STATS(stats->mem, prev_stats->mem, mem_reads);
	u64 mem_writes		= DIFF_STATS(stats->mem, prev_stats->mem, mem_writes);

	u64 freq, volt, cpu_energy, mem_energy;
	u64 target_cpu_energy, target_mem_energy, target_cpu_frequency, target_mem_frequency;
	int inefficiency_budget;
	read_inefficiency_budget(&inefficiency_budget);

	struct cpuEnergyFreq minCPUEnergyFreq;
	minCPUEnergyFreq = compute_cpu_emin(DIFF_STATS(stats->cpu, prev_stats->cpu, cpu_busy_cycles), DIFF_STATS(stats->cpu, prev_stats->cpu, cpu_idle_time));
	u64 cpu_emin		= minCPUEnergyFreq.energy;
	u64 cpu_fmin		= minCPUEnergyFreq.freq;
  	u64 cpu_vmin		= minCPUEnergyFreq.volt;

	struct memEnergyFreq minMemEnergyFreq;
	minMemEnergyFreq = compute_mem_emin(
			DIFF_STATS(stats->mem, prev_stats->mem, mem_actpreread_events),
			DIFF_STATS(stats->mem, prev_stats->mem, mem_actprewrite_events),
			DIFF_STATS(stats->mem, prev_stats->mem, mem_reads),
			DIFF_STATS(stats->mem, prev_stats->mem, mem_writes),
			DIFF_STATS(stats->mem, prev_stats->mem, mem_refresh_events),
			DIFF_STATS(stats->mem, prev_stats->mem, mem_active_time),
			DIFF_STATS(stats->mem, prev_stats->mem, mem_precharge_time),
			mem_curr_freq
			);
 	u64 mem_emin	=minMemEnergyFreq.energy;
 	u64 mem_fmin	=minMemEnergyFreq.freq;
	u64 net_emin  = 1;
	u64 total_budget = (cpu_emin + mem_emin + stats->net.net_emin) * (u64) inefficiency_budget;

	tuning_library_log("Emins : %llu %llu %llu\n", cpu_emin, mem_emin, stats->net.net_emin);
	tuning_library_log("Total budget               :%llu\n", total_budget);
	tuning_library_log("Quantum time(clock)        :%llu\n", quantum_time);
	tuning_library_log("Total time(diff)           :%llu\n", DIFF_STATS(stats->cpu, prev_stats->cpu, cpu_total_time));

	double cpu_load = ((double) cpu_busy_time / (double) quantum_time);
	component_settings->inefficiency[CPU] = cpu_load * cpu_max_inefficiency;
	component_settings->inefficiency[CPU] = component_settings->inefficiency[CPU] < 1000 ? 1000 : component_settings->inefficiency[CPU];
	component_settings->inefficiency[CPU] = component_settings->inefficiency[CPU] > cpu_max_inefficiency ? cpu_max_inefficiency : component_settings->inefficiency[CPU];
	tuning_library_log("CPU busy time              :%llu\n", cpu_busy_time);
	tuning_library_log("CPU idle time              :%llu\n", cpu_idle_time);
	tuning_library_log("CPU load                   :%f\n", cpu_load);
	tuning_library_log("CPU inefficiency (load)    :%d\n", component_settings->inefficiency[CPU]);

	double mem_load = ((double) mem_busy_time / (double) quantum_time);
	mem_load += 0.3;																//XXX: Hack
	component_settings->inefficiency[MEM] = mem_load * mem_max_inefficiency;
	component_settings->inefficiency[MEM] = component_settings->inefficiency[MEM] < 1000 ? 1000 : component_settings->inefficiency[MEM];
	component_settings->inefficiency[MEM] = component_settings->inefficiency[MEM] > mem_max_inefficiency ? mem_max_inefficiency : component_settings->inefficiency[MEM];
	tuning_library_log("MEM busy time              :%llu\n", mem_busy_time);
	tuning_library_log("MEM reads                  :%llu\n", mem_reads);
	tuning_library_log("MEM writes                 :%llu\n", mem_writes);
	tuning_library_log("MEM load(+30)              :%f\n", mem_load);
	tuning_library_log("MEM inefficiency (load)    :%d\n", component_settings->inefficiency[MEM]);

	component_settings->inefficiency[NET] = 1000;

	double num_parts = component_settings->inefficiency[CPU] + component_settings->inefficiency[MEM];		//XXX:Add network

	while(1) {
		int lhs = (component_settings->inefficiency[CPU] * cpu_emin) +
				  (component_settings->inefficiency[MEM] * mem_emin) +
				  (component_settings->inefficiency[NET] * net_emin);

		if(lhs > total_budget) {
			component_settings->inefficiency[CPU] -= (component_settings->inefficiency[CPU] / num_parts);
			if(component_settings->inefficiency[CPU] < 1000)
				component_settings->inefficiency[CPU] = 1000;

			component_settings->inefficiency[MEM] -= (component_settings->inefficiency[MEM] / num_parts);
			if(component_settings->inefficiency[MEM] < 1000)
				component_settings->inefficiency[MEM] = 1000;
		}
		else if(lhs <= total_budget)
			break;

		if(component_settings->inefficiency[CPU] == 1000 && component_settings->inefficiency[MEM] == 1000 && component_settings->inefficiency[NET] == 1000)
			break;
	}

	tuning_library_log("CPU target inefficiency:%d\t	MEM target inefficiency:%d\n",component_settings->inefficiency[CPU],component_settings->inefficiency[MEM] );

	// Best frequency matching target cpu inefficiency
	target_cpu_energy = component_settings->inefficiency[CPU] * cpu_emin / 1000;	//inefficiency is currently in millis
	for(freq=cpu_fmin, volt=cpu_vmin; freq <=CPUmaxFreq; freq+=CPUfStep, volt+=CPUVStep ) {
		cpu_energy = compute_cpu_energy (DIFF_STATS(stats->cpu, prev_stats->cpu, cpu_busy_cycles), DIFF_STATS(stats->cpu, prev_stats->cpu, cpu_idle_time), freq, volt);
		if( target_cpu_energy < cpu_energy)
			break;
	}
	if(freq > CPUminFreq)
		target_cpu_frequency = freq - CPUfStep;

	//Best frequency matching target mem inefficiency
	target_mem_energy = component_settings->inefficiency[MEM] * mem_emin / 1000;	//inefficiency is currently in millis
	target_mem_frequency = map_mem_energy_to_frequency(
			DIFF_STATS(stats->mem, prev_stats->mem, mem_actpreread_events),
			DIFF_STATS(stats->mem, prev_stats->mem, mem_actprewrite_events),
			DIFF_STATS(stats->mem, prev_stats->mem, mem_reads),
			DIFF_STATS(stats->mem, prev_stats->mem, mem_writes),
			DIFF_STATS(stats->mem, prev_stats->mem, mem_refresh_events),
			DIFF_STATS(stats->mem, prev_stats->mem, mem_active_time),
			DIFF_STATS(stats->mem, prev_stats->mem, mem_precharge_time),
			mem_fmin,
			target_mem_energy,
			mem_curr_freq);

	component_settings->frequency[CPU]	= target_cpu_frequency;
	component_settings->frequency[MEM]	= target_mem_frequency;
	mem_new_freq = target_mem_frequency;
}

static void run_tuning_algorithm(int signal) {
	struct stats stats;
	struct component_settings component_settings;

	read_stats(&stats);
	compute_inefficiency_targets(&stats, prev_stats, &component_settings);

	u64 cur_time = prev_stats->cur_time;
	bzero(prev_stats, sizeof(struct stats));
	prev_stats->cur_time = cur_time;

	write_stats("0");
	write_controller(&component_settings);

	schedule();
}

int tuning_library_init() {
	my_pid = getpid();
	if(my_pid < 0) {
		return -EINVAL;
	}

	signal(SIGALRM, run_tuning_algorithm);

	logbuf = malloc(sizeof(char) * LOGSIZE);
	bzero(logbuf, LOGSIZE);
	printf("Initialized Tuning library log\n");

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

	if(read_power_agile_components()) {
		printf("Failed to initialize tuning library\n");
		is_tuning_disabled = 1;
		return -1;
	}

	prev_stats = malloc(sizeof(struct stats));
	bzero(prev_stats, sizeof(prev_stats));

	is_init_complete = 1;

	return 0;
}

void tuning_library_start() {
	is_tuning_disabled = 0;
	if(is_init_complete)
		run_tuning_algorithm(0);
}

void tuning_library_stop() {
	is_tuning_disabled = 1;
}

void tuning_library_exit() {
	fprintf(stdout, "%s", logbuf);
	free(logbuf);
	is_tuning_disabled = 1;
}

void tuning_library_set_budget(int val) {
	write_inefficiency_budget(val);
}

void tuning_library_spec_init(int *argc_ptr, char ***argv_ptr) {
//  Tuning library hacks
    if(*argc_ptr > 1) {
        if(strcmp(*argv_ptr[1], "-poweragile") == 0) {
//          We're expecting another argument
            int budget = atoi(*argv_ptr[2]);
//          We were never here!
            is_tuning_disabled = 0;
            printf("argc :%d\n", *argc_ptr);
            printf("Tuning library enabled! Budget :%d\n", budget);
            *argv_ptr[1] = NULL;
            *argv_ptr[2] = NULL;
            int i;
            for(i = 3; i < *argc_ptr; i++) {
                *argv_ptr[i - 2] = *argv_ptr[i];
                *argv_ptr[i] = NULL;
            }
            printf("Next argument :%s\n", *argv_ptr[1]);
            tuning_library_init();
            tuning_library_set_budget(budget);
            tuning_library_start();
            *argc_ptr = *argc_ptr - 2;
        }
    }
}
