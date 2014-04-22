#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#include "include/common.h"
#include "include/perf.h"
#include "include/tuning_library.h"

int run_micro_benchmark(int argc, char **argv) {
/* mem.operation_run takes a little more than 10ms for
every 50K mem accesses. Each mem.operation_run results in
64 memAccesses which take about 20us (experimental value)
*/

//trying to generate a workload of varying phases
// cpu intensive, memory intensive, cpu + memory mix and idle

	//100% load
	cpu.timed_run(0.1);	//10ms

	//80%load
	mem.operation_run(100);	//2ms
	cpu.timed_run(0.08);	//8ms

	//60% cpuload
	mem.operation_run(200);	//4ms
	cpu.timed_run(0.06);	//6ms

	//40% cpuload
	mem.operation_run(300);	//6ms
	cpu.timed_run(0.04);	//4ms

	//20%cpu load
	mem.operation_run(400);	//8ms
	cpu.timed_run(0.02);	//2ms

//dummy runs
	mem.operation_run(500);
	mem.operation_run(500);
	mem.operation_run(500);
	mem.operation_run(500);


	//0% cpuload
	mem.operation_run(500);	//10ms

	//20%cpu load
	mem.operation_run(400);	//8ms
	cpu.timed_run(0.02);	//2ms

	//40% cpuload
	mem.operation_run(300);	//6ms
	cpu.timed_run(0.04);	//4ms

	//60% cpuload
	mem.operation_run(200);	//4ms
	cpu.timed_run(0.06);	//6ms
	
//dummy runs
	mem.operation_run(500);
	mem.operation_run(500);
	mem.operation_run(500);
	mem.operation_run(500);

	//80%load
	mem.operation_run(100);	//2ms
	cpu.timed_run(0.08);	//8ms

	//100%load
	cpu.timed_run(0.1);	//10ms

	//20%cpu load
	mem.operation_run(400);	//8ms
	cpu.timed_run(0.02);	//2ms

	sleep(0.01);
	//80%load
	mem.operation_run(100);	//2ms
	cpu.timed_run(0.08);	//8ms

	//40% cpuload
	mem.operation_run(300);	//6ms
	cpu.timed_run(0.04);	//4ms

	//60% cpuload
	mem.operation_run(200);	//4ms
	cpu.timed_run(0.06);	//6ms

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
