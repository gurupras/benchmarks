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

static int manual_annotations;
double cpu_load;


static void usage(char *error) {
	struct _IO_FILE *file = stdout;

	if(strcmp(error, " ") != 0)
		file = stderr;
	fprintf(file, "%s\n"
			"bench mem <option>\n"
			"Benchmarks the memory by executing loads and stores\n"
			"\nOPTIONS:\n"
			"    -h        : Print this help message\n"
			"    -n <n>    : Specifies number of memory accesses(loads and stores)\n"
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
		case 'm' :
			manual_annotations = 1;
			VERBOSE("Using manual annotations\n");
			break;
		case 'c' :
			cpu_load_flag = 1;
			cpu_load = atof(optarg);
			if(cpu_load > 1.0 || cpu_load < 0.0)
				usage("0.0 <= load <= 1.0\n");
			break;
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
	}
	else {
		if(!budget)
			usage("Must specify -b with -u\n");
	}

	if(!cpu_load_flag) {
		usage("Must specify -c\n");
	}

	printf("\n");

	return 0;
}



static int annotation_test(int argc, char **argv) {
	struct component_settings settings;
	double mem_load;
	u64 operations = 0;
	u64 duration = (10 * 1000000ULL) * multiplier;
	operations = ((1 - cpu_load) * duration) / MEM_OPERATION_DURATION;
	mem.init();
	mem_load = 1 - cpu_load;

	parse_opts(argc, argv);
	if(!is_tuning_disabled) {
		tuning_library_init();
		tuning_library_set_budget(budget);
		if(!manual_annotations)
			tuning_library_start();
	}

	VERBOSE("Starting annotation test\n");
	VERBOSE("Duration :%llu\n", duration);
	VERBOSE("CPU load :%.4f\n", cpu_load);
	VERBOSE("MEM load :%.4f\n", mem_load);
	VERBOSE("operations :%llu\n", operations);

	if(!is_tuning_disabled && manual_annotations) {
		settings.inefficiency[CPU] = CPU_MAX_INEFFICIENCY *
			cpu_load;
		settings.inefficiency[MEM] = MEM_MAX_INEFFICIENCY *
			mem_load;
		settings.inefficiency[NET] = 1000;
		tuning_library_force_annotation(settings);
	}
	mem.operation_run(operations);

	if(!is_tuning_disabled && manual_annotations) {
		settings.inefficiency[CPU] = CPU_MAX_INEFFICIENCY *
			1;
		settings.inefficiency[MEM] = MEM_MAX_INEFFICIENCY *
			0;
		settings.inefficiency[NET] = 1000;
		tuning_library_force_annotation(settings);
	}
	cpu.timed_run((cpu_load * duration));

	return 0;
}

struct benchmark annotation_test_benchmark = {
		.run				= annotation_test,
		.usage 				= NULL,
		.timed_run 			= NULL,
		.operation_run 		= NULL,
		.alarm_handler 		= NULL,
		.gracefully_exit 	= NULL,
};
