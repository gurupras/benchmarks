/*
 * main.c
 *
 *  Created on: May 31, 2013
 *      Author: guru
 */

#include <include/common.h>

static void usage() {
	printf("bench <benchmark> <options> \n"
			"    <benchmark> :\n"
			"        cpu     : Runs a CPU benchmark\n"
			"        mem     : Runs a memory benchmark\n"
			"        io      : Runs an IO benchmark\n"
			"        -h      : Print this help message\n"
			"\n"
			"<options> are specific to each benchmark\n"
			"To get list of options, type 'bench <benchmark> -h'\n"
			);
	exit(0);
}

static void filter_args(int argc, char **argv, int *new_argc, char **new_argv) {
	int i = 2;
	*new_argc = 0;

	while(i < argc) {
		*new_argc = *new_argc + 1;
		i++;
	}

	for(i = 0; i < *new_argc; i++) {
		strcpy(argv[i], argv[i+2]);
	}

	new_argv = argv;
}

int main(int argc, char **argv) {
	if(argc < 2)
		usage();

	error_msg = malloc(sizeof(char) * 1024);

	int i, j;
	for(i = 0; i < argc; i++) {
		for(j = 0; j < strlen(argv[i]); j++) {
			argv[i][j] = tolower(argv[i][j]);
		}
	}

	if(strcmp(argv[1], "cpu") == 0) {
		bench_cpu(argc, argv);
	}

	else if(strcmp(argv[1], "mem") == 0) {
		bench_mem(argc, argv);
	}

	else if(strcmp(argv[1], "io") == 0) {
		bench_io(argc, argv);
	}

	else if(strcmp(argv[1], "-h") == 0) {
		usage();
	}

	else {
		bzero(error_msg, strlen(error_msg));
		sprintf(error_msg, "Unknown benchmark '%s'", argv[1]);
		panic();
	}

	return 0;
}
