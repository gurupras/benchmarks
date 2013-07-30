/*
 * test.c
 * 
 * Tests kernel handling of shared private data pages.
 * 
 * (C) Stephen C. Tweedie <sct@redhat.com>, 1998
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "misc_lib.h"
#include <include/common.h>

#include <pthread.h>

int memory	= 8;	/* Default --- use 8 MB of heap */
int rprocesses	= 8;	/* Number of reader processes */
int wprocesses	= 1;	/* Number of writer processes */
int do_fork	= 0;	/* Enable random fork/die if true */

static struct list *pids;	/* Track child PIDs */
pthread_mutex_t mutex;	/* Mutex semaphore */

int pagesize;

char *progname;

char *heap;		/* Allocated space for a large working set */
int *patterns;		/* Record the patterns we expect in each heap page */
int nr_pages;		/* How many pages are in the heap */

static void setup_memory(void);		/* Setup the heap and pattern arrays*/
static void fork_child(int n, int writer);	/* Spawn a new child process */
static void fork_new_child(void);		/* Perform a fork/die
                                                   in this child */
static void run_test(int writer);		/* Run the test sweep
                                                   within a child */
static void null_handler(int);			/* Just a null signal handler */
static void page_error(int);			/* Report a heap
                                                   pattern mismatch */

#define page_val(page) (* (int *) (heap + pagesize * (page)))

static void usage(char *error) {
	struct _IO_FILE *file = stdout;

	if(error != " ")
		file = stderr;
	fprintf(file, "%s\n"
			"bench mem <option>\n"
			"Benchmarks the memory by running memtest until conditions specified have been satisfied\n"
			"\nOPTIONS:\n"
			"    -h        : Print this help message\n"
			"    -t <n>    : Specifies a time limit (in seconds)\n"
			"    -m <n>    : Specifies the memory heap size (in Megabytes)\n"
			"    -d <n>    : Number of readers\n"
			"    -w <n>    : Number of writers\n"
			"    -s <n>    : Add an idle duration for every 1000 numbers tested\n"
//			"    -r <n>    : Repeat benchmark and print average   (max:100)\n"
			, error);
	if(error)
		exit(-1);
}


static int gracefully_exit() {
		printf("Time elapsed  :%0.6fs\n", ((finish_time - start_time) / 1e9) );
		exit(0);
}

static void alarm_handler(int signal) {
	finish_time = rdclock();
	int *entry;

	printf("Killing child processes\n");
	for_each_entry(entry, pids) {
		kill(*entry, SIGKILL);
	}

	gracefully_exit();
}




int parse_opts(int argc, char *argv[])
{
	int c, i;

	progname = argv[0];
	
	while (c = getopt(argc, argv, "fhm:r:w:t:d:s:"), c != EOF) {
		switch (c) {
		case ':':
			usage("missing argument");
		case '?':
			usage("unrecognised argument");
		case 'h':
			usage(" ");
		case 'm':
			memory = strtoul(optarg, 0, 0);
			break;
		case 'd':
			rprocesses = strtoul(optarg, 0, 0);
			break;
		case 'w':
			wprocesses = strtoul(optarg, 0, 0);
			break;
		case 't' : {
			end_time = (ull) (atof(optarg) * 1e9);
			time_t sec 	= end_time / 1e9;
			time_t usec = ( (end_time - (sec * 1e9)) / 1e3);
			timeout_timer.it_interval.tv_usec = 0;
			timeout_timer.it_interval.tv_sec  = 0;
			timeout_timer.it_value.tv_sec     = sec;
			timeout_timer.it_value.tv_usec     = usec;
			break;
		}
		case 's' : {
			end_time = (ull) (atof(optarg) * 1e9);
			time_t sec 	= end_time / 1e9;
			time_t nsec = ( (end_time - (sec * 1e9)) );
			sleep_interval->tv_nsec = nsec;
			sleep_interval->tv_sec  = sec;
			break;
		}
		case 'r' :
//			repeat_count = atoi(optarg);
			break;

		default:
			usage("unknown error");
		}
	}
}

static int benchmark(int argc, char **argv) {
	parse_opts(argc, argv);

	init_list(pids)
	pthread_mutex_init(&mutex, NULL);

	int i;

	fprintf (stderr, "Starting test run with %d megabyte heap.\n", memory);

	setup_memory();

	signal(SIGALRM, alarm_handler);


	for (i=0; i<rprocesses; i++)
		fork_child(i, 0);
	for (; i<rprocesses+wprocesses; i++)
		fork_child(i, 1);

	fprintf (stderr, "%d child processes started.\n", i);

	setitimer(ITIMER_REAL, &timeout_timer, 0);
	start_time = rdclock();

	for (;;) {
		pid_t pid;
		int status;

		/* Catch child error statuses and report them. */
		pid = wait3(&status, 0, 0);
		if (pid < 0)	/* No more children? */
			exit(0);
		if (WIFEXITED (status)) {
			if (WEXITSTATUS (status))
				fprintf (stderr,
					 "Child %d exited with status %d\n",
					 pid, WEXITSTATUS(status));
			else
				fprintf (stderr,
					 "Child %d exited with normally\n",
					 pid);
		} else {
			fprintf (stderr,
				 "Child %d exited with signal %d\n",
				 pid, WTERMSIG(status));
		}
	}
}

static void setup_memory(void)
{
	int i;
	
	pagesize = getpagesize();
	nr_pages = memory * 1024 * 1024 / pagesize;

	fprintf (stderr, "Setting up %d %dkB pages for test...", 
		 nr_pages, pagesize);
	
	patterns = safe_malloc(nr_pages * sizeof(*patterns),"setup_memory");
	heap	 = safe_malloc(nr_pages * pagesize, "setup_memory");
	
	for (i=0; i<nr_pages; i++) {
		page_val(i) = i;
		patterns[i] = i;
	}

	fprintf (stderr, " done.\n");
	
}

static void fork_child(int n, int writer)
{
	pid_t pid, parent;
	
	parent = getpid();
	signal (SIGUSR1, null_handler);

	pid = fork();
	if (pid) {
		/* Are we the parent?  Wait for the child to print the
		   startup banner. */
		pause();
		append(pids, pid);		/* Append to pid list */

	} else {
		/* Are we the child?  Print a banner, then signal the parent
		   to continue. */
		fprintf (stderr, "Child %02d started with pid %05d, %s\n", 
			 n, getpid(), writer ? "writer" : "readonly");
		usleep(10);
		kill (parent, SIGUSR1);
		run_test(writer);
		/* The test should never terminate.  Exit with an error if
		   it does. */
		exit(2);
	}
}

static void run_test(int writer)
{
	int count = 0;
	int time_to_live = 0;
	int page;
	
	/* Give each child a different random seed. */
	srandom(getpid() * time(0));
	
	for (;;) {
		/* Track the time until the next fork/die round */
		if (do_fork) {
			if (time_to_live) {
				if (!--time_to_live)
					fork_new_child();
			}
			else
				time_to_live = random() % 50;
		}
					
		/* Pick a page and check its contents. */
		page = ((unsigned) random()) % nr_pages;
		if (page_val(page) != patterns[page])
			page_error(page);
		nanosleep(sleep_interval, NULL);
		/* Writer tasks should modify pages occasionally, too. */
		if (writer && count++ > 10) {
			count = 0;
			patterns[page] = page_val(page) = random();
		}
	}
}

static void fork_new_child(void)
{
	int old_pid = getpid();
	int pid;
	
	pid = fork();
	if (pid == -1) {
		perror("fork");
		exit(errno);
	}
	
	if (pid) {
		/* Are we the parent?  Wait for the child to print the
		   fork banner. */
		pause();
		exit(0);
	} else {
		/* Are we the child?  Print a banner, then signal the parent
		   to continue. */
		fprintf (stderr, "Child %05d forked into pid %05d\n", 
			 old_pid, getpid());
		kill (old_pid, SIGUSR1);
	}
}


static void null_handler(int n)
{
}

static void page_error(int page)
{
	fprintf (stderr, 
		 "Child %05d failed at page %d, address %p: "
		 "expected %08x, found %08x\n",
		 getpid(), page, &page_val(page),
		 patterns[page], page_val(page));
	exit(3);
}

struct benchmark mem = {
	.usage				= usage,
	.parse_opts			= parse_opts,
	.alarm_handler		= alarm_handler,
	.gracefully_exit	= gracefully_exit,
	.run				= benchmark,
};
