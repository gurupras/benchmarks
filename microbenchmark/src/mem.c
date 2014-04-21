#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

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
	int startOffset;

	printf("memBmark: Obtained start time.. Starting loop . . .\n");
//	asm volatile("tight_loop_start: nop");

	for(i=0; i<loops; i++) {
		memfreq_test_loop();
	}
//	asm volatile("tight_loop_finish: nop");
	printf("memBmark: Obtained end time.. \n");
}

static void dummy_mem_frequency_test_run()
{
	int i;

	memArrayCopy = memArray;
	
	asm volatile("dummy_memory_loop_start: nop");

	for(i=0; i<5; i++) {
		memfreq_test_loop();
	}
	asm volatile("dummy_memory_loop_finish: nop");
}

void mem_init() {
	int i,j;

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
void run_memBmark(int loops) {

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
