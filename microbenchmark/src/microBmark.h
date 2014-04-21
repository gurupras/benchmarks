#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define unlikely(x)             __builtin_expect(!!(x), 0)
#define likely(x)               __builtin_expect(!!(x), 1)

inline unsigned long long rdclock(void) {
	struct timespec ts; 

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
	return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

