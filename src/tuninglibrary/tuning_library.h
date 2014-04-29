/*
 * tuning_library.h
 *
 *  Created on: Apr 19, 2014
 *      Author: guru
 */

#ifndef TUNING_LIBRARY_H_
#define TUNING_LIBRARY_H_

typedef unsigned long long u64;
typedef unsigned int u32;

int tuning_library_init(void);
void tuning_library_start(void);
void tuning_library_stop(void);
void tuning_library_exit(void);
void tuning_library_set_interval(unsigned int val);
void tuning_library_set_budget(int val);


#endif /* TUNING_LIBRARY_H_ */
