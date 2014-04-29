#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#include "include/common.h"
#include "include/perf.h"
#include "tuninglibrary/tuning_library.h"


#define GOVERNOR_POLL_INTERVAL	((u64) (10 * MSEC_TO_NSEC))
#define MEM_OPERATION_DURATION	((u64) (20 * USEC_TO_NSEC))

int run_micro_benchmark(int argc, char **argv) {
/* mem.operation_run takes a little more than 10ms for
every 50K mem accesses. Each mem.operation_run results in
64 memAccesses which take about 20us (experimental value)
*/

//trying to generate a workload of varying phases
// cpu intensive, memory intensive, cpu + memory mix and idle

	int cpu_load;

	int multiplier = 2;
	if(argc == 3)
		multiplier = atoi(argv[2]);
	u64 duration = GOVERNOR_POLL_INTERVAL * multiplier;
	u64 operations = 0;

	mem.init();
	printf("Starting micro benchmark\n");

	for(cpu_load = 100; cpu_load >= 0; cpu_load -= 20) {
		double cpu_load_double = cpu_load / (double) 100;
		double current_duration = cpu_load_double * duration;
		operations = ((1 - cpu_load_double) * duration) / MEM_OPERATION_DURATION;
		mem.operation_run(operations);	//2ms
		printf("Starting cpu benchmark with load :%f  duration :%f\n", cpu_load_double, current_duration);

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
		printf("Starting cpu benchmark with load :%f\n", cpu_load_double);
		cpu.timed_run(cpu_load_double * duration);
		printf("\n");
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

struct benchmark micro_benchmark = {
		.run				= run_micro_benchmark,
		.usage 				= NULL,
		.timed_run 			= NULL,
		.operation_run 		= NULL,
		.alarm_handler 		= NULL,
		.gracefully_exit 	= NULL,
};
