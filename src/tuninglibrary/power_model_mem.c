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

#include "power_model_mem.h"

unsigned int mem_init_done = 1;
unsigned int mem_inefficiency=2000;

struct memWorkStats memWorkStatsArray[MEM_SLIDING_WINDOW_LENGTH];
unsigned int mem_head=0, mem_tail=0;

//Memory Timing and Power Model parameters
u64 tBURST; //in ps
u64 IDD0, IDD2N, IDD3N, IDD4R, IDD4W, IDD5B;
u64 mem_prev_freq=0, mem_freq;
u32 mem_actpreread_events, mem_actprewrite_events, mem_reads, mem_writes;
u32 mem_precharge_time, mem_active_time, mem_refresh_events;
u64 mem_energy;
u32 minLatency= 4*tRP;
u64 mem_prev_time, mem_time_curr;
u64 mem_prev_pid=0xFFFF;
u64 mem_prev_energy=0;
/*
u64 convert_mem_inefficiency_to_energy(u64 actpreread_events, u64 actprewrite_events, u64 reads, u64 writes, u64 refreshes, u64 active_time, u64 precharge_time, int inefficiency) {
	u64 emin, energy;
	//call compute_mem_emin();
	// compute energy from ineff and emin
	emin = compute_mem_emin(actpreread_events, actprewrite_events, reads, writes, refreshes, active_time, precharge_time, mem_prev_freq);
	energy= (emin * inefficiency) / 1000; //inefficiency is in milli
	return energy;
}
*/
//exhaustive search for frequency settings
u64 map_mem_energy_to_frequency(u64 actpreread_events, u64 actprewrite_events, u64 num_reads, u64 num_writes, u64 num_refreshes, u64 active_time, u64 precharge_time, u64 min_freq, u64 mem_energy_tgt, u64 mem_freq) {
	u64 freq;
	u64 energy, read_burst_time, total_time;
	u64 active_time_local, read_burst_time_local;

	mem_prev_freq = mem_freq;
	read_burst_time = ((num_reads + num_writes) * tBURSTSpec * memfSpec) / mem_prev_freq; //in ps
	read_burst_time = read_burst_time / 1000; // in ns

	//active time is samped based on current tick. So, if the sampling is in between
	//current read, then only part of active time is sampled and the next one is counted 
	//in the next sampling period. And, as the read is also counted in the next sampling period,
	// active_time can be < than read_burst_time
/*	if(read_burst_time > active_time) {
		printf("panic: read_burst_time:%llu   active_time:%llu\n",read_burst_time, active_time);
		panic("read_burst_time > active_time \n");
	}
*/
	for(freq=min_freq; freq<=maxMemFreq; freq+=memfStep) {
		active_time_local =0;

		//scaling performance
		if(active_time > read_burst_time)
			active_time_local = active_time - read_burst_time;
		read_burst_time_local = (read_burst_time * mem_prev_freq) / freq; // in ns
		active_time_local += read_burst_time_local;
/*
		//valid for only close page policy
		if((1000*active_time_local) < ((num_reads + num_writes)*minLatency)) {
			active_time_local = (num_reads + num_writes)*minLatency;
			active_time_local = active_time_local / 1000;//in ns
		}
*/
		total_time = active_time_local+precharge_time; //accounts for two ranks
		num_refreshes = (total_time * 1000) / tREFI;

		energy = compute_mem_energy(actpreread_events, actprewrite_events, num_reads, num_writes, num_refreshes, active_time_local, precharge_time, freq);

		if(mem_energy_tgt < energy)
			break;
	}
	if(freq > minMemFreq)
		freq -= memfStep;

	return freq;
}

//following function optimizes search for frequency settings for close page policy
/**********THIS FUNCTION IS BROKEN ***********/
u64 map_mem_energy_to_frequency_close( u64 actpreread_events, u64 actprewrite_events, u64 num_reads, u64 num_writes, u64 num_refreshes, u64 active_time, u64 precharge_time, u32 inefficiency, u64 mem_energy_tgt) {
	u64 freq;
	u64 energy, read_burst_time, total_time;
	u64 active_time_local, read_burst_time_local;
 	unsigned int i,numSteps;

	read_burst_time = ((num_reads+num_writes) * tBURSTSpec * memfSpec) / mem_prev_freq; //in ps
	read_burst_time = read_burst_time / 1000; // in ns
	//active time is samped based on current tick. So, if the sampling is in between
	//current read, then only part of active time is sampled and the next one is counted 
	//in the next sampling period. And, as the read is also counted in the next sampling period,
	// active_time can be < than read_burst_time
/*	if(read_burst_time > active_time) {
		printf("panic: read_burst_time:%llu   active_time:%llu\n",read_burst_time, active_time);
		panic("read_burst_time > active_time \n");
	}
*/

	numSteps = ((inefficiency - 1) * 10) / 5;
	if(numSteps==0) {
		freq = minMemFreq;
		return freq;
	} else if(numSteps > maxfSteps) {
		freq = maxMemFreq;
		return freq;
	} else if(numSteps <0) {
		printf("Panic: Memory Inefficiency is < 1\n");
	}
	freq = minMemFreq + (numSteps-1)*memfStep;
	for(i=0; i<3; i++) {  
		scale_mem_perf_power_model(freq);
		active_time_local =0;

		//scaling performance
		if(active_time > read_burst_time)
			active_time_local = active_time - read_burst_time;

		read_burst_time_local = (read_burst_time * mem_prev_freq) / freq; // in ns
		active_time_local += read_burst_time_local;

		if((1000*active_time_local) < ((num_reads + num_writes)*minLatency)) {
			active_time_local = (num_reads + num_writes)*minLatency;
			active_time_local = active_time_local / 1000;//in ns
		}

		total_time = active_time_local+precharge_time; //accounts for two ranks
		num_refreshes = (total_time * 1000) / tREFI;

		energy = compute_mem_energy(actpreread_events, actprewrite_events, num_reads, num_writes, num_refreshes, active_time_local, precharge_time, freq);

		if(mem_energy_tgt < energy)
			break;
		freq += memfStep;
		if(freq > maxMemFreq)
			break;
	}
	//if target frequency falls below range
	if(freq == (minMemFreq + (numSteps-1)*memfStep)) {
//		printf("mem ineff. contr.: target frequency falls below range\n");
		for(freq=minMemFreq; freq < (minMemFreq+(numSteps*memfStep)); freq+=memfStep) {
			scale_mem_perf_power_model(freq);

			//scaling performance
			active_time_local = active_time - read_burst_time;
			read_burst_time_local = (read_burst_time * mem_prev_freq) / freq; // in ns
			active_time_local += read_burst_time_local;

			if((1000*active_time_local) < ((num_reads + num_writes)*minLatency)) {
				active_time_local = (num_reads + num_writes)*minLatency;
				active_time_local = active_time_local / 1000;//in ns
			}

			total_time = active_time_local+precharge_time; //accounts for two ranks
			num_refreshes = (total_time * 1000) / tREFI;

			energy = compute_mem_energy(actpreread_events, actprewrite_events,num_reads, num_writes, num_refreshes, active_time_local, precharge_time, freq);

			if(mem_energy_tgt < energy)
				break;
		}
		if(freq > minMemFreq)
			freq -=memfStep;
		return freq;
	}
	//if target falls above range
	if(freq > (minMemFreq + (numSteps+1)*memfStep)) {
//		printf("mem ineff. contr.: target frequency falls above range\n");
		for(freq= (minMemFreq+(numSteps+2)*memfStep); freq <= maxMemFreq; freq+=memfStep) {
			scale_mem_perf_power_model(freq);

			//scaling performance
			active_time_local = active_time - read_burst_time;
			read_burst_time_local = (read_burst_time * mem_prev_freq)  / freq; // in ns
			active_time_local += read_burst_time_local;

			if((1000*active_time_local) < ((num_reads + num_writes)*minLatency)) {
				active_time_local = (num_reads + num_writes)*minLatency;
				active_time_local = active_time_local / 1000;//in ns
			}

			total_time = active_time_local+precharge_time; //accounts for two ranks
			num_refreshes = (total_time * 1000) / tREFI;

			energy = compute_mem_energy(actpreread_events, actprewrite_events,num_reads, num_writes, num_refreshes, active_time_local, precharge_time, freq);

			if(mem_energy_tgt < energy)
				break;
		}
		freq -=memfStep;
		return freq;
	}
	if(freq > minMemFreq)
		freq -= memfStep;

	return freq;
}

struct memEnergyFreq compute_mem_emin(u64 actpreread_events, u64 actprewrite_events,u64 num_reads, u64 num_writes, u64 num_refreshes, u64 active_time, u64 precharge_time, u32 mem_freq) {
	u64 freq, emin=0, fmin;
	u64 energy, read_burst_time, total_time;
	u64 active_time_local, read_burst_time_local;

	struct memEnergyFreq minEnergyFreq;

	mem_prev_freq = mem_freq;
	read_burst_time = ((num_reads+num_writes) * tBURSTSpec * memfSpec) / mem_prev_freq; //in ps
	read_burst_time = read_burst_time / 1000; // in ns
	//active time is samped based on current tick. So, if the sampling is in between
	//current read, then only part of active time is sampled and the next one is counted 
	//in the next sampling period. And, as the read is also counted in the next sampling period,
	// active_time can be < than read_burst_time
/*	if(read_burst_time > active_time) {
		printf("panic: read_burst_time:%llu   active_time:%llu\n",read_burst_time, active_time);
		panic("read_burst_time > active_time \n");
	}
*/
	
	freq = minMemFreq;//emin is always at minMemFreq
	for(freq=minMemFreq; freq<=maxMemFreq; freq+=memfStep) {
///		scale_mem_perf_power_model(freq);
		active_time_local =0;

		//scaling performance
		if(active_time > read_burst_time)
			active_time_local = active_time - read_burst_time;
		read_burst_time_local = (read_burst_time * mem_prev_freq) / freq; // in ns
		active_time_local += read_burst_time_local;
		
		/*
		//valid for only close page policy
		if((1000*active_time_local) < ((num_reads + num_writes)*minLatency)) {
			active_time_local = (num_reads + num_writes)*minLatency;
			active_time_local = active_time_local / 1000;//in ns
		}
		*/
		total_time = active_time_local+precharge_time; //accounts for two ranks
		num_refreshes = (total_time * 1000) / tREFI;

		energy = compute_mem_energy(actpreread_events, actprewrite_events,num_reads, num_writes, num_refreshes, active_time_local, precharge_time, freq);
		if(freq == minMemFreq) {
			emin = energy;
			fmin = minMemFreq;
		} else if(energy <= emin) {
			emin = energy;
			fmin = freq;
		}
//		printf("mem ineff. contr. (emin): energy:%llu    freq:%llu\n", energy, freq);
	}
	if(fmin != minMemFreq)
		printf("ineff. contrl. mem: fmin != minMemFreq -- fmin:%llu	minMemFreq:%d\n",fmin, minMemFreq);
	minEnergyFreq.energy = emin;
	minEnergyFreq.freq = fmin;
	return minEnergyFreq;
}

u64 compute_mem_energy(u64 actpreread_events,u64  actprewrite_events,u64 reads, u64 writes, u64 refreshes, u64 active_time, u64 precharge_time,  u64 freq) {
	u64 energy=0;
	u64 refresh_energy, read_energy, write_energy, background_energy;
	u64 read_precharge_energy, read_burst_energy, write_precharge_energy, write_burst_energy;

	scale_mem_perf_power_model(freq);

	//calculate refresh energy
	refresh_energy = refreshes * (IDD5B - IDD3N) * tRFC * Vdd; // c * mA * ps * mV
	refresh_energy = refresh_energy / 1000000000; //in nJ
	refresh_energy = refresh_energy / 1000;//in uJ

	//calculate act/pre energy for reads
	read_precharge_energy = actpreread_events * ((IDD0 * 4*tRP) - ((IDD3N * 3*tRP) + (IDD2N* tRP))) * Vdd; // mA * ps *mV
	read_precharge_energy = read_precharge_energy / 1000000000; //in nJ
	read_precharge_energy = read_precharge_energy / 1000; //in uJ

	//calculate act/pre energy for writes
	write_precharge_energy = actprewrite_events * ((IDD0 * 4*tRP) - ((IDD3N * 3*tRP) + (IDD2N* tRP))) * Vdd;// mA * ps *mV
	write_precharge_energy = write_precharge_energy / 1000000000; //in nJ
	write_precharge_energy = write_precharge_energy / 1000; //in uJ
     
	//calculate read burst energy
	read_burst_energy = reads * ((IDD4R - IDD3N)* tBURST) * Vdd; //mA * ps * mV
	read_burst_energy = read_burst_energy / 1000000000; //in nJ
	read_burst_energy = read_burst_energy / 1000; //in uJ

	//calculate write burst energy
	write_burst_energy = writes * ((IDD4W - IDD3N)* tBURST) * Vdd; //mA * ps * mV
	write_burst_energy = write_burst_energy / 1000000000; //in nJ
	write_burst_energy = write_burst_energy / 1000; //in uJ

	//compute read energy
	read_energy = read_precharge_energy + read_burst_energy; //in uJ

	//compute write energy
	write_energy = write_precharge_energy + write_burst_energy; // in uJ
	
	//compute background energy
	background_energy = ((precharge_time * IDD2N) + (active_time * IDD3N)) * Vdd; // ns * mA * mV
	background_energy = background_energy / 1000000; // in nJ
	background_energy = background_energy / 1000; // in uJ

	//compute total energy
	energy = refresh_energy + read_energy + write_energy + background_energy;

/*	printf("calculating energy\n");
	printf("reads:%llu	writes:%llu	refreshes:%llu	active_time:%llu	precharge_time:%llu	freq:%llu\n",reads,writes,refreshes,active_time,precharge_time,freq);
	printf("IDD0:%llu	IDD2N:%llu	IDD3N:%llu	IDD5B:%llu	IDD4R:%llu	IDD4W:%llu\n",IDD0,IDD2N,IDD3N,IDD5B,IDD4R,IDD4W);
	printf("Vdd:%d	tRFC:%d		tRP:%d		tBURST:%llu\n",Vdd,tRFC,tRP,tBURST);
	printf("refresh:%llu	read_pre:%llu	write_pre:%llu	read_burst:%llu	   write_burst:%llu   read:%llu	   write:%llu   background:%llu	 total:%llu\n",refresh_energy,read_precharge_energy,write_precharge_energy,read_burst_energy,write_burst_energy,read_energy,write_energy,background_energy,energy);
*/	
	//printf("mem ineff. contr. (energy): energy:%llu    freq:%llu\n", energy, freq);
	return energy;	
}

void scale_mem_perf_power_model(u64 freq_curr) {

	u64 tCK_curr;
	u64 IDD0_temp, IDD5B_temp;

	tCK_curr = 1000000 / freq_curr; // in ps

	//timing parameters scaling
	/*tCL, tRCD , tRP, tRFC, tREFI, tXAW are characteristics of asynchronous DRAM module
	while tBURST is always 4 clock cycles and therefore latency(time) of 4 clock cycles
	changes with frequency, while rest of the timings remain constant. */
	tBURST = (tBURSTSpec * tCK_curr) / tCKSpec;

	//current paramter scaling   
	IDD0_temp = ((IDD0Spec * 4*tRP) - ((IDD3NSpec * 3*tRP) + (IDD2NSpec* tRP))) ;  
	IDD5B_temp = IDD5BSpec - IDD3NSpec;

	IDD2N = (IDD2NSpec * tCKSpec) / tCK_curr;
	IDD3N = (IDD3NSpec * tCKSpec) / tCK_curr;
	IDD4W = (IDD4WSpec * tCKSpec) / tCK_curr;
	IDD4R = (IDD4RSpec * tCKSpec) / tCK_curr;

	IDD0 = (IDD0_temp  + ((IDD3N * 3 * tRP) + (IDD2N * tRP))) / (4 * tRP);
	IDD5B = IDD5B_temp + IDD3N;         //actual refresh current doesn't scale with frequency
}

struct memWorkStats normalize_mem_stats(u64 actpreread_events,u64  actprewrite_events,u64 reads, u64 writes, u64 refreshes, u64 active_time, u64 precharge_time) {
	struct memWorkStats normStats;
	u64 norm_factor,norm_period;
	u64 total_time;

	total_time = active_time + precharge_time;
	norm_period = MEM_NORMALIZATION_PERIOD;
	norm_factor = (norm_period * 1000) / total_time;

	normStats.actpreReadEvents = actpreread_events * norm_factor;
	normStats.actpreWriteEvents = actprewrite_events * norm_factor;
	normStats.numReads = reads * norm_factor;
	normStats.numWrites = writes * norm_factor;
	normStats.refreshEvents = refreshes * norm_factor;
	normStats.activeTime = active_time * norm_factor;
	normStats.prechargeTime = precharge_time *norm_factor;

	normStats.actpreReadEvents = normStats.actpreReadEvents / 1000;
	normStats.actpreWriteEvents = normStats.actpreWriteEvents / 1000;
	normStats.numReads= normStats.numReads / 1000;
	normStats.numWrites = normStats.numWrites / 1000;
	normStats.refreshEvents = normStats.refreshEvents / 1000;
	normStats.activeTime = normStats.activeTime / 1000;
	normStats.prechargeTime = normStats.prechargeTime / 1000;
        
        return normStats;
}

