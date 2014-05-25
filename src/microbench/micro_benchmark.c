#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#include "include/common.h"
#include "include/perf.h"
#include "include/tuning_library.h"


#define GOVERNOR_POLL_INTERVAL	((u64) (10 * MSEC_TO_NSEC))
#define MEM_OPERATION_DURATION	((u64) (20 * USEC_TO_NSEC))

static int budget;
static int multiplier;


static void usage(char *error) {
	struct _IO_FILE *file = stdout;

	if(strcmp(error, " ") != 0)
		file = stderr;
	fprintf(file, "%s\n"
			"bench mem <option>\n"
			"Benchmarks the memory by executing loads and stores\n"
			"\nOPTIONS:\n"
			"    -h        : Print this help message\n"
//			"    -t <n>    : Specifies a time limit (in nanoseconds)\n"
			"    -n <n>    : Specifies number of memory accesses(loads and stores)\n"
//			"    -l        : UNIMPLEMENTED: Use memory loads only (default is mixed)\n"
//			"    -s        : UNIMPLEMENTED: Use memory stores only (default is mixed)\n"
//			"    -d <n>    : Set stride length to <n> bytes\n"
			"    -u        : Enable the power-agile tuning library\n"
			"    -b        : Assign inefficiency budget\n"
			"\nNOTE:\n"
			"-n must be set. \n"
			"If unset, the program terminates as there is no break condition\n"
			, error);

	if(strcmp(error, " ") != 0)
		exit(-1);
}

static int parse_opts(int argc, char **argv) {
	int opt;

	while( (opt = getopt(argc, argv, "n:b:hu")) != -1) {
		switch(opt) {
		case ':' :
			usage("missing parameter value");
			break;
		case 'n' :
			multiplier = atoi(optarg);
			break;
		case 'h' :
			usage(" ");
			break;
		case 'u' :
			is_tuning_disabled = 0;
			break;
		case 'b' :
			budget = atoi(optarg);
			break;
		default :
		case '?' :
			usage("invalid command line argument");
			break;
		}
	}
	return 0;
}


int run_micro_benchmark(int argc, char **argv) {
/* mem.operation_run takes a little more than 10ms for
every 50K mem accesses. Each mem.operation_run results in
64 memAccesses which take about 20us (experimental value)
*/

//trying to generate a workload of varying phases
// cpu intensive, memory intensive, cpu + memory mix and idle

	parse_opts(argc, argv);
	tuning_library_init();
	tuning_library_set_budget(budget);
	tuning_library_start();

	int cpu_load;

	u64 duration = GOVERNOR_POLL_INTERVAL * multiplier;
	u64 operations = 0;

	mem.init();
	printf("Starting micro benchmark\n");

	struct component_settings settings;

	settings.inefficiency[CPU] = 4300;
	settings.inefficiency[MEM] = 1400;
	settings.inefficiency[NET] = 1000;
	tuning_library_force_annotation(settings);
	for(cpu_load = 100; cpu_load >= 0; cpu_load -= 20) {
		double cpu_load_double = cpu_load / (double) 100;
		double current_duration = cpu_load_double * duration;
		operations = ((1 - cpu_load_double) * duration) / MEM_OPERATION_DURATION;
		mem.operation_run(operations);	//2ms
		printf("Starting cpu benchmark with load :%f  duration :%f\n", cpu_load_double, current_duration);

		cpu.timed_run(cpu_load_double * duration);	//10ms
		printf("\n");
	}

	settings.inefficiency[CPU] = 1200;
	settings.inefficiency[MEM] = 2600;
	settings.inefficiency[NET] = 1000;
	tuning_library_force_annotation(settings);

//dummy runs
	mem.operation_run(500);
	mem.operation_run(500);
	mem.operation_run(500);
	mem.operation_run(500);

	settings.inefficiency[CPU] = 4300;
	settings.inefficiency[MEM] = 1400;
	settings.inefficiency[NET] = 1000;
	tuning_library_force_annotation(settings);
	for(cpu_load = 0; cpu_load <= 100; cpu_load += 20) {
		double cpu_load_double = cpu_load / (double) 100;
		operations = ((1 - cpu_load_double) * duration) / MEM_OPERATION_DURATION;
		mem.operation_run(operations);
		printf("Starting cpu benchmark with load :%f\n", cpu_load_double);
		cpu.timed_run(cpu_load_double * duration);
		printf("\n");
	}

	settings.inefficiency[CPU] = 4300;
	settings.inefficiency[MEM] = 1400;
	settings.inefficiency[NET] = 1000;
	tuning_library_force_annotation(settings);
//dummy runs
	mem.operation_run(500);
	mem.operation_run(500);
	mem.operation_run(500);
	mem.operation_run(500);

	double cpu_load_double;
	settings.inefficiency[CPU] = 4300;
	settings.inefficiency[MEM] = 1400;
	settings.inefficiency[NET] = 1000;
	tuning_library_force_annotation(settings);
	//80%load
	cpu_load_double = 80 / (double) 100;
	operations = ((1 - cpu_load_double) * duration) / MEM_OPERATION_DURATION;
	mem.operation_run(operations);
	cpu.timed_run((cpu_load_double * duration));

	settings.inefficiency[CPU] = 4300;
	settings.inefficiency[MEM] = 1200;
	settings.inefficiency[NET] = 1000;
	tuning_library_force_annotation(settings);
	//100%load
	cpu_load_double = 100 / (double) 100;
	operations = ((1 - cpu_load_double) * duration) / MEM_OPERATION_DURATION;
	mem.operation_run(operations);
	cpu.timed_run((cpu_load_double * duration));

	settings.inefficiency[CPU] = 1500;
	settings.inefficiency[MEM] = 4300;
	settings.inefficiency[NET] = 1000;
	tuning_library_force_annotation(settings);
	//20%cpu load
	cpu_load_double = 20 / (double) 100;
	operations = ((1 - cpu_load_double) * duration) / MEM_OPERATION_DURATION;
	mem.operation_run(operations);
	cpu.timed_run((cpu_load_double * duration));

	settings.inefficiency[CPU] = 4300;
	settings.inefficiency[MEM] = 1400;
	settings.inefficiency[NET] = 1000;
	tuning_library_force_annotation(settings);
	//80%load
	cpu_load_double = 80 / (double) 100;
	operations = ((1 - cpu_load_double) * duration) / MEM_OPERATION_DURATION;
	mem.operation_run(operations);
	cpu.timed_run((cpu_load_double * duration));

	//40% cpuload
	cpu_load_double = 40 / (double) 100;
	operations = ((1 - cpu_load_double) * duration) / MEM_OPERATION_DURATION;
	mem.operation_run(operations);
	cpu.timed_run((cpu_load_double * duration));

	//60% cpuload
	cpu_load_double = 60 / (double) 100;
	operations = ((1 - cpu_load_double) * duration) / MEM_OPERATION_DURATION;
	mem.operation_run(operations);
	cpu.timed_run((cpu_load_double * duration));

	settings.inefficiency[CPU] = 4300;
	settings.inefficiency[MEM] = 1400;
	settings.inefficiency[NET] = 1000;
	tuning_library_force_annotation(settings);
	//80%load
	cpu_load_double = 80 / (double) 100;
	operations = ((1 - cpu_load_double) * duration) / MEM_OPERATION_DURATION;
	mem.operation_run(operations);
	cpu.timed_run((cpu_load_double * duration));

	settings.inefficiency[CPU] = 4300;
	settings.inefficiency[MEM] = 1400;
	settings.inefficiency[NET] = 1000;
	tuning_library_force_annotation(settings);
	//100%load
	cpu_load_double = 100 / (double) 100;
	operations = ((1 - cpu_load_double) * duration) / MEM_OPERATION_DURATION;
	mem.operation_run(operations);
	cpu.timed_run((cpu_load_double * duration));

	//20%cpu load
	cpu_load_double = 20 / (double) 100;
	operations = ((1 - cpu_load_double) * duration) / MEM_OPERATION_DURATION;
	mem.operation_run(operations);
	cpu.timed_run((cpu_load_double * duration));

	settings.inefficiency[CPU] = 4300;
	settings.inefficiency[MEM] = 1400;
	settings.inefficiency[NET] = 1000;
	tuning_library_force_annotation(settings);
	//80%load
	cpu_load_double = 80 / (double) 100;
	operations = ((1 - cpu_load_double) * duration) / MEM_OPERATION_DURATION;
	mem.operation_run(operations);
	cpu.timed_run((cpu_load_double * duration));

	//40% cpuload
	cpu_load_double = 40 / (double) 100;
	operations = ((1 - cpu_load_double) * duration) / MEM_OPERATION_DURATION;
	mem.operation_run(operations);
	cpu.timed_run((cpu_load_double * duration));

	//60% cpuload
	cpu_load_double = 60 / (double) 100;
	operations = ((1 - cpu_load_double) * duration) / MEM_OPERATION_DURATION;
	mem.operation_run(operations);
	cpu.timed_run((cpu_load_double * duration));

	tuning_library_exit();
	return 0;
}

struct benchmark micro_benchmark = {
		.run				= run_micro_benchmark,
		.usage 				= NULL,
		.timed_run 			= NULL,
		.operation_run 		= NULL,
		.alarm_handler 		= NULL,
		.gracefully_exit 	= NULL,
};
