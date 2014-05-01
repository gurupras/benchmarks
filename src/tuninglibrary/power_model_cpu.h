/*
*  linux/include/linux/memfreq.h
*
*  Copyright (C) 2013 Guru Prasad <gurupras@buffalo.edu
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/

#include "tuning_library.h"

struct cpuWorkStats 
{
u64 cpuBusyCycles;
u64 cpuIdleTime;
u64 time;
};

struct cpuEnergyFreq
{
u64 energy;
u64 freq;
u64 volt;
};

#define CPU_SLIDING_WINDOW_LENGTH 10
#define CPU_NORMALIZATION_PERIOD 10000000	//10ms .. LUT is designed for this period

//CPU Power Model
#define CPUPleakSpec	89	//mW
#define CPUPbkgndSpec	208	//mW
#define CPUPdynSpec	190	//mW
#define CPUfSpec	1000	//MHz
#define CPUVSpec	1250	//mV
#define CPUfStep	30	//MHz
#define CPUVStep	20	//mV
#define CPUminFreq	100	//MHz
#define CPUmaxFreq	1000	//MHz
#define CPUminVolt	650	//mV
#define CPUmaxVolt	1250	//mV
//lookup table
#define cpuIdleTimeStep		500000	//in ns (0.5ms)  
#define cpuBusyCyclesStep	100000	//100k cycles
#define cpuEnergyArrayLength	31	//due to 31 frequency steps 


u64 convert_cpu_inefficiency_to_energy(u64, u64, int);
struct cpuWorkStats normalize_cpu_stats(u64, u64,u64);
u64 map_cpu_energy_to_frequency(u64, u64, u64);
struct cpuEnergyFreq compute_cpu_emin(u64, u64);
void create_cpu_energy_freq_lookup_table(void);
u64 compute_cpu_energy(u64,u64,u64,u64);
