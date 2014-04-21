#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include "microBmark.h"
#include "mem.c"
#include "cpu.c"

int main() {
	int i;
	uint32_t cpu_time=0;
	uint32_t mem_loops=0;
/* run_memBmark takes a little more than 10ms for 
every 50K mem accesses. Each run_memBmark results in 
64 memAccesses which take about 20us (experimental value)
*/
	mem_init();

//trying to generate a workload of varying phases
// cpu intensive, memory intensive, cpu + memory mix and idle

	//100% load
	run_bench_cpu(0.1);	//10ms

	//80%load
	run_memBmark(100);	//2ms
	run_bench_cpu(0.08);	//8ms

	//60% cpuload
	run_memBmark(200);	//4ms
	run_bench_cpu(0.06);	//6ms

	//40% cpuload
	run_memBmark(300);	//6ms
	run_bench_cpu(0.04);	//4ms

	//20%cpu load
	run_memBmark(400);	//8ms
	run_bench_cpu(0.02);	//2ms

//dummy runs
	run_memBmark(500);
	run_memBmark(500);
	run_memBmark(500);
	run_memBmark(500);


	//0% cpuload
	run_memBmark(500);	//10ms

	//20%cpu load
	run_memBmark(400);	//8ms
	run_bench_cpu(0.02);	//2ms

	//40% cpuload
	run_memBmark(300);	//6ms
	run_bench_cpu(0.04);	//4ms

	//60% cpuload
	run_memBmark(200);	//4ms
	run_bench_cpu(0.06);	//6ms
	
//dummy runs
	run_memBmark(500);
	run_memBmark(500);
	run_memBmark(500);
	run_memBmark(500);

	//80%load
	run_memBmark(100);	//2ms
	run_bench_cpu(0.08);	//8ms

	//100%load
	run_bench_cpu(0.1);	//10ms

	//20%cpu load
	run_memBmark(400);	//8ms
	run_bench_cpu(0.02);	//2ms

	sleep(0.01);
	//80%load
	run_memBmark(100);	//2ms
	run_bench_cpu(0.08);	//8ms

	//40% cpuload
	run_memBmark(300);	//6ms
	run_bench_cpu(0.04);	//4ms

	//60% cpuload
	run_memBmark(200);	//4ms
	run_bench_cpu(0.06);	//6ms

	mem_exit();
	return 0;
}
