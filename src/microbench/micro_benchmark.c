#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#include "include/common.h"
#include "include/perf.h"
#include "tuning_library.h"


#define GOVERNOR_POLL_INTERVAL	((u64) (10 * MSEC_TO_NSEC))
#define MEM_OPERATION_DURATION	((u64) (20 * USEC_TO_NSEC))

static int budget;
static int multiplier;

static int short_run, manual_annotations;
double cpu_manual_load;

static void short_micro_benchmark(double);

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
			"    -q        : Quick run (only runs 60/20/100%% CPU loads\n"
			"    -m        : Use manual annotations (with -q)\n"
			"    -c [0-1]  : CPU load to simulate [0.0 - 1.0]\n"
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
	int cpu_load_flag = 0;

	while( (opt = getopt(argc, argv, "n:b:c:mhuqv")) != -1) {
		switch(opt) {
		case ':' :
			usage("missing parameter value");
			break;
		case 'n' :
			multiplier = atoi(optarg);
			VERBOSE("Multiplier :%d\n", multiplier);
			break;
		case 'h' :
			usage(" ");
			exit(0);
		case 'u' :
			is_tuning_disabled = 0;
			VERBOSE("Tuning library is enabled\n");
			break;
		case 'b' :
			budget = atoi(optarg);
			VERBOSE("Budget :%d\n", budget);
			break;
		case 'q' :
			short_run = 1;
			VERBOSE("Using short micro benchmark\n");
			break;
		case 'm' :
			manual_annotations = 1;
			VERBOSE("Using manual annotations\n");
			break;
		case 'c' :
			cpu_load_flag = 1;
			cpu_manual_load = atof(optarg);
			if(cpu_manual_load > 1.0 || cpu_manual_load < 0.0)
				usage("0.0 <= load <= 1.0\n");
		case 'v' :
			verbose = 1;
			break;
		default :
		case '?' :
			usage("invalid command line argument");
			break;
		}
	}


	if(is_tuning_disabled) {
		if(budget)
			printf("Warning: Ignoring budget as tuning is disabled\n");
		if(manual_annotations)
			printf("Warning: Ignoring annotations as tuning is disabled\n");
		if(short_run || manual_annotations) {
			usage("Options require -u flag\n");
		}
	}
	else {
		if(!budget)
			usage("Must specify -b with -u\n");
	}

	if(short_run && !cpu_load_flag) {
		usage("Currently must specify -c with -q\n");
	}

	printf("\n");

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
	if(!is_tuning_disabled) {
		tuning_library_init();
		tuning_library_set_budget(budget);
		if(!manual_annotations)
			tuning_library_start();
	}

	if(short_run) {
		short_micro_benchmark(cpu_manual_load);
		exit(0);
	}

	int cpu_load;

	u64 duration = GOVERNOR_POLL_INTERVAL * multiplier;
	u64 operations = 0;

	mem.init();
	VERBOSE("Starting micro benchmark\n");

	struct component_settings settings;

	if(!is_tuning_disabled) {
		settings.inefficiency[CPU] = 4300;
		settings.inefficiency[MEM] = 1400;
		settings.inefficiency[NET] = 1000;
		tuning_library_force_annotation(settings);
	}

	for(cpu_load = 100; cpu_load >= 0; cpu_load -= 20) {
		double cpu_load_double = cpu_load / (double) 100;
		double current_duration = cpu_load_double * duration;
		operations = ((1 - cpu_load_double) * duration) / MEM_OPERATION_DURATION;
		mem.operation_run(operations);	//2ms
		VERBOSE("Starting cpu benchmark with load :%f  duration :%f\n", cpu_load_double, current_duration);

		cpu.timed_run(cpu_load_double * duration);	//10ms
		printf("\n");
	}



//dummy runs
	mem.operation_run(500);
	mem.operation_run(500);
	mem.operation_run(500);
	mem.operation_run(500);


	for(cpu_load = 0; cpu_load <= 100; cpu_load += 20) {
		double cpu_load_double = cpu_load / (double) 100;
		operations = ((1 - cpu_load_double) * duration) / MEM_OPERATION_DURATION;
		mem.operation_run(operations);
		VERBOSE("Starting cpu benchmark with load :%f\n", cpu_load_double);
		cpu.timed_run(cpu_load_double * duration);
		VERBOSE("\n");
	}



//dummy runs
	mem.operation_run(500);
	mem.operation_run(500);
	mem.operation_run(500);
	mem.operation_run(500);

	double cpu_load_double;

	//80%load
	cpu_load_double = 80 / (double) 100;
	operations = ((1 - cpu_load_double) * duration) / MEM_OPERATION_DURATION;
	mem.operation_run(operations);
	cpu.timed_run((cpu_load_double * duration));

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

	//80%load
	cpu_load_double = 80 / (double) 100;
	operations = ((1 - cpu_load_double) * duration) / MEM_OPERATION_DURATION;
	mem.operation_run(operations);
	cpu.timed_run((cpu_load_double * duration));

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

	return 0;
}

static void short_micro_benchmark(double cpu_load_double) {
	struct component_settings settings;
	double mem_load_double;
	u64 duration = GOVERNOR_POLL_INTERVAL * multiplier;
	u64 operations = 0;

	VERBOSE("Starting short micro benchmark\n");
	mem.init();
	/*
	 * The tuning library has already been initialized.
	 * just proceed with benchmark
	 */
	mem_load_double = 1 - cpu_load_double;

	VERBOSE("CPU load :%.4f\n", cpu_load_double);
	VERBOSE("MEM load :%.4f\n", mem_load_double);

	if(!is_tuning_disabled && manual_annotations) {
		settings.inefficiency[CPU] = CPU_MAX_INEFFICIENCY *
			cpu_load_double;
		settings.inefficiency[MEM] = MEM_MAX_INEFFICIENCY *
			mem_load_double;
		settings.inefficiency[NET] = 1000;
		tuning_library_force_annotation(settings);
	}
	operations = ((1 - cpu_load_double) * duration) / MEM_OPERATION_DURATION;
	mem.operation_run(operations);
	cpu.timed_run((cpu_load_double * duration));
}

struct benchmark micro_benchmark = {
		.run				= run_micro_benchmark,
		.usage 				= NULL,
		.timed_run 			= NULL,
		.operation_run 		= NULL,
		.alarm_handler 		= NULL,
		.gracefully_exit 	= NULL,
};
