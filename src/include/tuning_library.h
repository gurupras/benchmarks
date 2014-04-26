/*
 * tuning_library.h
 *
 *  Created on: Apr 19, 2014
 *      Author: guru
 */

#ifndef TUNING_LIBRARY_H_
#define TUNING_LIBRARY_H_

#define unlikely(x)		__builtin_expect(!!(x), 0)
#define likely(x)		__builtin_expect(!!(x), 1)

int tuning_library_init(void);
void tuning_library_start(void);
void tuning_library_stop(void);
void tuning_library_set_interval(unsigned int val);
void tuning_library_set_budget(unsigned int val);

#endif /* TUNING_LIBRARY_H_ */
