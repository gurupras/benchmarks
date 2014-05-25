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

enum COMPONENT {
	CPU,
	MEM,
	NET,
	NR_COMPONENTS,
};

struct component_settings {
	int inefficiency[NR_COMPONENTS];
	int frequency[NR_COMPONENTS];
};

extern unsigned int is_tuning_disabled;

int tuning_library_init(void);
void tuning_library_spec_init(int *argc_ptr, char ***argv_ptr);
void tuning_library_start(void);
void tuning_library_stop(void);
void tuning_library_exit(void);
void tuning_library_set_interval(unsigned int val);
void tuning_library_set_budget(int val);
void tuning_library_force_annotation(struct component_settings settings);


#endif /* TUNING_LIBRARY_H_ */