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


void perf_handler(int);
void perf_reset_stats(void);
void perf_init(void);

void perf_read_stats(u64 *, u64 *, u64 *);

#endif /* PERF_H_ */
