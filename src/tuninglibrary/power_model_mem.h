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

struct memWorkStats 
{
u64 actpreReadEvents;
u64 actpreWriteEvents;
u64 numReads;
u64 numWrites;
u64 refreshEvents;
u64 prechargeTime;
u64 activeTime;
};

#define MEM_SLIDING_WINDOW_LENGTH 10
#define MEM_NORMALIZATION_PERIOD 10000000	//10ms .. LUT is designed for this period


//Memory Timing and Power Model
// DDR3-1600 11-11-11
#define tRCD  13750 //in ps
#define tCL   13750 // in ps
#define tRP   13750 //in ps
#define tBURSTSpec  5000 //in ps
#define tRFC  300000 //in ps
#define tREFI  7800000 // in ps
#define memfSpec  800 //MHz
#define tCKSpec  1250 //in ps clock period

//current values; took for x16 device and multiplied by 4 for x64.
// all current values are in mA and voltage in mV
#define IDD0Spec   340 // #85*4
#define IDD2NSpec  180 // #45*4
#define IDD3NSpec  200 // #50*4
#define IDD4RSpec  760 //#190*4 -- this number is valid for BL8, for BL4 it should be doubled and = 1520
#define IDD4WSpec  820 //#205*4 -- this number is valid for BL8, for BL4 it should be doubled and = 1640
#define IDD5BSpec  680 //#170*4
#define Vdd    1500 //mV

//specs
#define minMemFreq  200
#define maxMemFreq  800
#define memfStep  100
#define maxfSteps 6 

//lookup table

u64 convert_mem_inefficiency_to_energy(u64, u64, u64, u64, u64, u64, u64, int);
struct memWorkStats normalize_mem_stats(u64, u64, u64, u64,u64,u64,u64);
u64 map_mem_energy_to_frequency(u64, u64, u64, u64, u64, u64, u64, u32, u64);
u64 map_mem_energy_to_frequency_close(u64, u64, u64, u64, u64, u64, u64, u32, u64);
u64 compute_mem_emin(u64, u64, u64, u64, u64, u64, u64, u32);
u64 compute_mem_energy(u64, u64, u64,u64,u64,u64,u64,u64);
void scale_mem_perf_power_model(u64);
