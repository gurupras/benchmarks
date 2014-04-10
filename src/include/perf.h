/*
 * periodic_perf.h
 *
 *  Created on: Apr 10, 2014
 *      Author: guru
 */

#ifndef PERF_H_
#define PERF_H_

#include "common.h"

extern unsigned int periodic_perf;


void periodic_perf_handler(int);
void reset_periodic_perf_stats(void);
void periodic_perf_init(void);

ull perf_read_periodic_perf_cycles(void);
ull perf_read_periodic_perf_insts(void);
ull perf_read_periodic_perf_cache_miss(void);
void perf_read_power_agile_task_stats(char *buf);

#endif /* PERF_H_ */
