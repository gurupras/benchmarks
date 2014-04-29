/*
 * mem.c
 *
 *  Created on: Apr 9, 2014
 *      Author: guru
 */


#include "include/common.h"
#include "include/perf.h"
#include "tuninglibrary/tuning_library.h"

#ifdef ARM

static int is_finished = 0;
static int is_tuning_enabled = 0;
static int budget;

static u64 num_loops;

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
			"    -p        : Enable performance counter accounting\n"
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

	while( (opt = getopt(argc, argv, "t:n:d:b:hlspu")) != -1) {
		switch(opt) {
		case ':' :
			usage("missing parameter value");
			break;
		case 't' : {
			end_time = (u64) (atof(optarg) * 1e9);
			time_t sec 	= end_time / 1e9;
			time_t usec = ( (end_time - (sec * 1e9)) / 1e3);
			timeout_timer.it_interval.tv_usec = 0;
			timeout_timer.it_interval.tv_sec  = 0;
			timeout_timer.it_value.tv_sec     = sec;
			timeout_timer.it_value.tv_usec     = usec;
			break;
		}
		case 'n' :
			num_loops = strtoull(optarg, 0, 0);
			break;
		case 'l' :
//			load_flag = 1;
			break;
		case 's' :
//			store_flag = 1;
			break;
		case 'd' :
//			stride_length = atoi(optarg);
			break;
		case 'h' :
			usage(" ");
			break;
		case 'p' :
//			Experimental periodic_perf
			periodic_perf = 1;
			break;
		case 'u' :
			is_tuning_enabled = 1;
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

//	if(load_flag && store_flag)
//		printf("Warning: loads and stores are both selected. Switching to mixed mode\n");
//	if(!load_flag && !store_flag) {
//		load_flag = 1;
//		store_flag = 1;
//	}
//	if(end_time == 0 && num_accesses == ~0) {
//		usage("No options specified\nTerminating program due to infinite loop!\n");
//		panic(" ");
//	}
	if(budget == 0) {
		usage("Must specify budget!\n");
		panic(" ");
	}

	return 0;
}

static int gracefully_exit() {
	printf("Time elapsed  :%0.6fs\n", ((finish_time - start_time) / 1e9) );
	if(periodic_perf) {
		perf_handler(SIGUSR1);
	}
	is_finished = 1;
	return 0;
}

static void alarm_handler(int signal) {
	finish_time = rdclock();
	gracefully_exit();
}

uint64_t *memArray, *memArrayCopy, arraySize;
unsigned long long memAccesses;
int accessesPerLoop, cacheBlockSize, step, stepInBytes;

static void memfreq_test_loop() {
	unsigned int value;
	unsigned int r3_value, r4_value, r5_value, r7_value, r10_value, r2_value,r6_value;
	(void) value;

//	Saving register state
	asm volatile("mov %0, r3" : "=r" (r3_value));
	asm volatile("mov %0, r4" : "=r" (r4_value));
	asm volatile("mov %0, r5" : "=r" (r5_value));
	asm volatile("mov %0, r7" : "=r" (r7_value));
	asm volatile("mov %0, r10" : "=r" (r10_value));
	asm volatile("mov %0, r2" : "=r" (r2_value));
	asm volatile("mov %0, r6" : "=r" (r6_value));

	asm volatile("mov %%r4, %0" : :"r" (memArrayCopy));
	asm volatile("mov %%r7, %0" : : "r" (stepInBytes));
        asm volatile("mov %%r6, %0" : : "r" (accessesPerLoop));
        asm volatile("mov %%r3, %0" : : "r" (step));

	asm volatile("mov r3, #0");
        asm volatile("mov r5, #0");
        asm volatile("mov r10, #0");

//	-----			Start core loop			-----
        asm volatile("array_loop: ldr r2, [r4,r3]");
        asm volatile("add r10,r10,r2");
        asm volatile("add r3,r3,r7");
        asm volatile("sub r6,r6,#1");
	asm volatile("cmp r6, r5");
	asm volatile("bne array_loop");

//	Restoring register state
	asm volatile("mov r3, %0" : : "r" (r3_value));
	asm volatile("mov r4, %0" : : "r" (r4_value));
	asm volatile("mov r5, %0" : : "r" (r5_value));
	asm volatile("mov r7, %0" : : "r" (r7_value));
	asm volatile("mov r10, %0" : : "r" (r10_value));
	asm volatile("mov r2, %0" : : "r" (r2_value));
	asm volatile("mov r6, %0" : : "r" (r6_value));

//	-----		End core loop			-----
}

static void calculate_mem_frequency(int loops)
{
	int i;

	printf("memBmark: Obtained start time.. Starting loop . . .\n");
//	asm volatile("tight_loop_start: nop");

	for(i=0; i<loops; i++) {
		memfreq_test_loop();
	}
//	asm volatile("tight_loop_finish: nop");
	printf("memBmark: Obtained end time.. \n");
}

void mem_init() {
	int i;

	arraySize = 524288;// 4MB
	// l2cache size is 2MB = 2097152bytes.
	// Minimum arraysize should be 2*2MB = 4194304bytes. 524288 elements;

	cacheBlockSize = 64; //64 bytes
	step = cacheBlockSize/8; // each cache block has 8 elements
	step = 8192;
	stepInBytes = step * 8; //8 bytes per element
	accessesPerLoop = arraySize/step;

	memArray = malloc(sizeof(uint64_t) * arraySize);
	for(i=0; i<arraySize; i++)
		memArray[i] = i;

	memArrayCopy = memArray;
}

void operation_run_mem(int loops) {

	memAccesses = loops * accessesPerLoop; //accessing every 16th element .. and loop repeats 5 times

	printf("Starting the benchmark\n");
	printf("arraySize is %llu\n",arraySize);
	printf("step is %d\n",step);
	printf("accessesPerLoop are %d\n",accessesPerLoop);
	printf("Loops are %d\n",loops);
	printf("Expected Mem access:%llu\n",memAccesses);

	calculate_mem_frequency(loops);
}

void mem_exit() {
	free(memArray);
	//free(memArrayCopy);
}

static int run_bench_mem(int argc, char **argv) {
	parse_opts(argc, argv);
	mem_init();
	if(is_tuning_enabled) {
		tuning_library_init();
		tuning_library_set_budget(budget);
		tuning_library_start();
	}

	operation_run_mem(num_loops);

	if(is_tuning_enabled)
		tuning_library_exit();

	return 0;
}

struct benchmark mem = {
	.init				= mem_init,
	.usage				= usage,
	.parse_opts			= parse_opts,
	.alarm_handler		= alarm_handler,
	.gracefully_exit	= gracefully_exit,
	.run				= run_bench_mem,
	.operation_run		= operation_run_mem,
};
#else
struct benchmark mem = {
	.init				= NULL,
	.usage				= NULL,
	.parse_opts			= NULL,
	.alarm_handler		= NULL,
	.gracefully_exit	= NULL,
	.run				= NULL,
	.operation_run		= NULL,
};
#endif
