/*
*  linux/arch/arm/mach-vexpress/memfreq.c
*
*  Copyright (C) 2012-2013 University at Buffalo.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* MEMFrequency functions
*/

#include <stdio.h>
#include <stdlib.h>

#include "power_model_cpu.h"

struct timespec time_ts={0,0};
struct cpuWorkStats cpuWorkStatsArray[CPU_SLIDING_WINDOW_LENGTH];
int head=0,tail=0;
int cpu_inefficiency=2000;
u64 cpu_energy=0,cpu_freq;
u64 prev_time=0;
u64 dummyIdx=0;
u32 cpu_busy_cycles_curr=0, cpu_idle_time_curr=0;
u64 time_curr=0;
u64 prev_pid=0xFFFF; //some huge number that can't be very first pid
u64 prev_freq=0;
u64 cpu_prev_energy=0;
//lookup table 
u64 ***cpuIdleTimeArray;
u64 cpuIdleTimeArrayLength, *cpuBusyCyclesArrayLength;

void create_cpu_energy_freq_lookup_table() {
	u_int32_t i,j,k;
	u64 **busyCycles_ptr,*energy_ptr;
	u64 idle_time, busy_time;
	u64 min_busy_cycles, max_busy_cycles, busy_cycles;
	u64 freq, volt;

	cpuIdleTimeArrayLength = (CPU_NORMALIZATION_PERIOD / cpuIdleTimeStep) + 1; // 10ms/0.5ms = 20+1 = 21
	cpuIdleTimeArray = malloc(sizeof(u64 *) * cpuIdleTimeArrayLength);
	cpuBusyCyclesArrayLength = malloc(sizeof(u64) * cpuIdleTimeArrayLength);

	//compute cpuBusyCycles array length for each idle time
	for(i=0; i<cpuIdleTimeArrayLength; i++) {
		idle_time = cpuIdleTimeStep * i; // in ns
		busy_time = CPU_NORMALIZATION_PERIOD - idle_time; //in ns
		busy_time = busy_time / 1000; //in us
		min_busy_cycles = busy_time * CPUminFreq;
		max_busy_cycles = busy_time * CPUmaxFreq;
		cpuBusyCyclesArrayLength[i] = ((max_busy_cycles - min_busy_cycles) / cpuBusyCyclesStep) + 1;
	}   
				
	//point idle time array to busy cycle arrays
	for(i=0; i<cpuIdleTimeArrayLength; i++)
		cpuIdleTimeArray[i] = malloc(sizeof(u64 *) * cpuBusyCyclesArrayLength[i]);
	
	//point busy cycle arrays to energy arrays
	for(i=0; i<cpuIdleTimeArrayLength; i++) {
		busyCycles_ptr = cpuIdleTimeArray[i];   //ptr to cpuBusyCycles array
		for(j=0; j<cpuBusyCyclesArrayLength[i]; j++) 
			busyCycles_ptr[j] = malloc(sizeof(u64) * cpuEnergyArrayLength);
	}

	//populate energy arrays
	for(i=0; i<cpuIdleTimeArrayLength; i++) {
		idle_time = cpuIdleTimeStep * i; // in ns
		busy_time = CPU_NORMALIZATION_PERIOD - idle_time; //in ns
		busy_time = busy_time /  1000; //in us
		min_busy_cycles = busy_time * CPUminFreq;
		max_busy_cycles = busy_time * CPUmaxFreq;
		busyCycles_ptr = cpuIdleTimeArray[i];   //ptr to cpuBusyCycles array
		for(j=0; j<cpuBusyCyclesArrayLength[i]; j++) {
			busy_cycles = min_busy_cycles + (j * cpuBusyCyclesStep);
			energy_ptr = busyCycles_ptr[j];
			freq = CPUminFreq;
			volt = CPUminVolt;
			for(k=0; k<cpuEnergyArrayLength; k++) {
				energy_ptr[k] = compute_cpu_energy(busy_cycles, idle_time, freq, volt);
				freq += CPUfStep;
				volt += CPUVStep;
			}
		}
	}
}

u64 convert_cpu_inefficiency_to_energy(u64 cpu_busy_cycles, u64 cpu_idle_time, int inefficiency) {
	u64 emin, energy;
	emin = compute_cpu_emin(cpu_busy_cycles, cpu_idle_time);
	energy = (emin * inefficiency) / 1000;//inefficiency is in milli
	return energy;
}

u64 map_cpu_energy_to_frequency(u64 cpu_busy_cycles, u64 cpu_idle_time, u64 energy) {
        u_int32_t i;
        u64 **busyCycles_ptr,*energy_ptr;
	u64 cpu_busy_time;
	u64 min_busy_cycles, max_busy_cycles;
	u64 freq, volt;
	u64 idleTimeIdx, busyCyclesIdx;

	idleTimeIdx = cpu_idle_time / cpuIdleTimeStep;
	if(idleTimeIdx >= cpuIdleTimeArrayLength) {
		printf("ineff. contr. : idleTimeIdx is :%llu\n",idleTimeIdx);
		printf("ineff. contr. : cpu_idle_time is :%llu\n",cpu_idle_time);
		printf("ineff. contr. : cpuIdleTimeStep : %d\n",cpuIdleTimeStep);
		printf("ineff. contr. : cpuIdleTimeArrayLength: %llu",cpuIdleTimeArrayLength);
		printf("ineff. contr. : cpu_busy_cycles: %llu\n",cpu_busy_cycles);
		//panic(KERN_DEBUG "ineff. contr. : idleTimeIdx is >= cpuIdleTimeArrayLength\n");
		printf("ineff. contr. : idleTimeIdx is >= cpuIdleTimeArrayLength\n");
		idleTimeIdx = cpuIdleTimeArrayLength -1;
	}
	busyCycles_ptr = cpuIdleTimeArray[idleTimeIdx]; //ptr to cpuBusyCycles array
	cpu_busy_time = CPU_NORMALIZATION_PERIOD - cpu_idle_time; //in ns
	cpu_busy_time = cpu_busy_time / 1000; //in us
	min_busy_cycles = cpu_busy_time * CPUminFreq;
	max_busy_cycles = cpu_busy_time * CPUmaxFreq;
	if((cpu_busy_cycles < 0)||(cpu_busy_cycles > 10000000)) {
		printf("ineff. contrl. cpuBusyCycles:%llu	\n",cpu_busy_cycles);
		printf("ineff. contrl. cpuIdleTime:%llu	\n",cpu_idle_time);
		printf("ineff. contrl. mincpuBusyCycles:%llu	\n",min_busy_cycles);
		printf("ineff. contrl. maxcpuBusyCycles:%llu	\n",max_busy_cycles);
//		panic("ineff. contr. : cpu_busy_cycles is out of range!\n");
		printf("ineff. contr. : cpu_busy_cycles is out of range!\n");
	}

	if(cpu_busy_cycles < min_busy_cycles)
		busyCyclesIdx=0;
	else				
		busyCyclesIdx = (cpu_busy_cycles - min_busy_cycles) / cpuBusyCyclesStep;
	if(busyCyclesIdx == cpuBusyCyclesArrayLength[idleTimeIdx])
		busyCyclesIdx--;
	else if(busyCyclesIdx > cpuBusyCyclesArrayLength[idleTimeIdx]) {
		printf("cpu_idle_time:%llu    cpu_busy_cycles: %llu    min_busy_cycles:%llu    max_busy_cycles:%llu\n",cpu_idle_time,cpu_busy_cycles, min_busy_cycles, max_busy_cycles);
		printf("cpuBusyCyclesStep: %d, busyCyclesIdx:%llu 	cpuBusyCyclesArrayLength[%llu]:%llu\n",cpuBusyCyclesStep, busyCyclesIdx, idleTimeIdx, cpuBusyCyclesArrayLength[idleTimeIdx]);
	
//		panic("ineff. contr. :busyCyclesIdx > cpuBusyCyclesArrayLength[idleTimeIdx]\n");
		printf("ineff. contr. :busyCyclesIdx > cpuBusyCyclesArrayLength[idleTimeIdx]\n");
		busyCyclesIdx = cpuBusyCyclesArrayLength[idleTimeIdx] - 1; 
	}

	energy_ptr = busyCycles_ptr[busyCyclesIdx];
	freq = CPUminFreq;
	volt = CPUminVolt;
	for(i=0; i<cpuEnergyArrayLength; i++) {
		if(energy_ptr[i] <= energy_ptr[i+1])
			break;
		freq += CPUfStep;
		volt += CPUVStep;
	}   

	if(energy > energy_ptr[i]) {
		for(i++; i<cpuEnergyArrayLength; i++) {
			if(energy < energy_ptr[i])
				break;
			freq += CPUfStep;
			volt += CPUVStep;
		}   
	}   
	return freq;
}

u64 compute_cpu_emin(u64 cpu_busy_cycles, u64 cpu_idle_time) {
	u64 freq, volt=CPUminVolt, emin=0;
	u64 energy;

	for(freq=CPUminFreq; freq<=CPUmaxFreq; freq+=CPUfStep, volt+=CPUVStep) {
		energy = compute_cpu_energy(cpu_busy_cycles, cpu_idle_time, freq, volt);
		if(freq == CPUminFreq) {
			emin = energy;
//			fmin = CPUminFreq;	//FIXME
		} else if(energy <= emin) {
			emin = energy;
//			fmin = freq;		//FIXME
		} else 
			break;
	}
	return emin;
}

u64 compute_cpu_energy(u64 cpu_busy_cycles, u64 cpu_idle_time, u64 freq, u64 volt) {
	u64 cpu_leakage_energy, cpu_background_energy, cpu_dynamic_energy, energy;
	u64 cpu_busy_time, total_time;
	u64 norm_freq, norm_volt;

	norm_freq = (freq * 1000) / CPUfSpec;
	norm_volt = (volt * 1000) / CPUVSpec;
	cpu_busy_time = (cpu_busy_cycles * 1000) /  freq; //cpu_busy_time in ns
	total_time = cpu_busy_time + cpu_idle_time;

	cpu_leakage_energy = CPUPleakSpec * norm_volt * total_time; //mw * ns *10^3
	cpu_leakage_energy = cpu_leakage_energy / 1000; //mW * ns
	cpu_leakage_energy = cpu_leakage_energy / 1000000; //uJ

	cpu_background_energy = CPUPbkgndSpec * norm_volt * norm_freq * norm_freq * total_time; //mW * 10^9 * ns
	cpu_background_energy = cpu_background_energy / 1000000000; //mW * ns
	cpu_background_energy = cpu_background_energy / 1000000; //uJ

	cpu_dynamic_energy = CPUPdynSpec *  norm_volt * norm_freq * norm_freq * cpu_busy_time; //mW * 10^9 *ns
	cpu_dynamic_energy = cpu_dynamic_energy / 1000000000;//mW * ns
	cpu_dynamic_energy = cpu_dynamic_energy / 1000000;//uJ

	energy = cpu_leakage_energy + cpu_background_energy + cpu_dynamic_energy;
	return energy;	
}

struct cpuWorkStats normalize_cpu_stats(u64 cpu_busy_cycles, u64 cpu_idle_time, u64 total_time) {
	struct cpuWorkStats normStats;
	u64 norm_factor,norm_period;

	norm_period = CPU_NORMALIZATION_PERIOD;
	norm_factor = (norm_period * 1000) / total_time;
	
	normStats.cpuBusyCycles = cpu_busy_cycles * norm_factor;
	normStats.cpuIdleTime = cpu_idle_time * norm_factor;
	normStats.cpuBusyCycles = normStats.cpuBusyCycles / 1000;
	normStats.cpuIdleTime = normStats.cpuIdleTime / 1000;
	normStats.time = CPU_NORMALIZATION_PERIOD;
        
        return normStats;
}

