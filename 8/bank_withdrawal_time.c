// Operating Systems: sample code  (c) Tomáš Hudec
// Threads
// Critical Sections
//
// Modified: 2015-12-10, 2018-11-05, 2023-03-29, 2023-11-28

#if !defined _XOPEN_SOURCE || _XOPEN_SOURCE < 600
#	define _XOPEN_SOURCE 600	// portable usage of barriers
#endif

#include <stdio.h>
#include <stdlib.h>				// srand(3), rand(3)
#include <sys/types.h>
#include <unistd.h>				// getpid()
#include <pthread.h>
#include <errno.h>
#include <stdbool.h>			// bool, true, false
#include <time.h>				// real time measuring
#include <sys/time.h>			// CPU time measuring
#include <sys/resource.h>		// CPU time measuring
#include <stdatomic.h>			// atomic_long
#include "cs_methods.h"			// methods for critical section access control

#define MAX_THREADS		1024

#define PER_THREAD		(1<<22)	// default transactions per thread
#define THREADS			(1<<3)	// default number of threads
#define WITHDRAW_AMOUNT	1		// amount per a transaction

bool do_sync_start = true;		// always synchronous start

long per_thread = PER_THREAD;	// transactions per thread
int thread_count = THREADS;		// the number of threads
volatile long balance;			// shared variable, initial balance
volatile atomic_long balance_atomic;	// used for atomic solution

long withdrawn[MAX_THREADS];	// the amount withdrawn by each thread

int verbose = 1;				// verbosity
int cs_method = -1;				// a command-line option

// synchronization variables
pthread_barrier_t sync_start_barrier;
bool sync_start_barrier_initialized = false;

struct timespec real_time1, real_time2;	// counting the real time
struct rusage CPU_time1, CPU_time2;		// counting the CPU clocks
double real_time;						// time spent executing the process
double CPU_time_user, CPU_time_system;	// time spent on the CPU
 
// prototypes
void eval_args(int argc, char *argv[]);

// release all allocated resources, used in atexit(3)
// uvolnění všech alokovaných prostředků, použito pomocí atexit(3)
void release_resources(void)
{       
	if (sync_start_barrier_initialized) {
		// destroy the barrier
		if ((errno = pthread_barrier_destroy(&sync_start_barrier)) != 0)
			perror("pthread_barrier_destroy");
		else
			sync_start_barrier_initialized = false;
	}
	// destroy resources used for the critical section access control
	cs_destroy();
}

FORCE_INLINE
void time_init(void)
{
	clock_gettime(CLOCK_MONOTONIC, &real_time1);	// real time init
	getrusage(RUSAGE_SELF, &CPU_time1);				// initialize the CPU time
}

// synchronize start of all threads
// synchronizace startu vláken
static void sync_threads(void)
{
	switch ((errno = pthread_barrier_wait(&sync_start_barrier))) {
		case PTHREAD_BARRIER_SERIAL_THREAD:
			if (verbose)
		      	printf("All threads have started transactions.\n");
			time_init();
		case 0:
			break;
		default:
			perror("pthread_barrier_wait");
			exit(3);
	}
}

// withdraw given amount, returns true if the transaction was successful, false otherwise
FORCE_INLINE
bool withdraw(long amount) {
	if (cs_method == CS_METHOD_ATOMIC) {	// use atomic type
		// check if the transaction can be done
		if (balance_atomic < amount)		// if not enough: reject withdrawal
			return false;
		balance_atomic -= amount;			// do withdrawal
	}
	else {
		// check if the transaction can be done
		if (balance < amount)		// if not enough: reject withdrawal
			return false;
		balance -= amount;			// do withdrawal
	}
	return true;
}

void *do_withdrawals(void *arg)
{
	#define	tid	(*(int *)arg)	// thread id from arg
	long amount;
	long i;

 	if (do_sync_start)
		sync_threads();			// synchronize start of all threads

	// each thread makes per_thread withdrawals
	for (i = 0; i < per_thread; ++i) {

		amount = WITHDRAW_AMOUNT;		// for the sake of measuring, it’s always the same

		cs_enter(tid);					// critical section begin

		if (withdraw(amount))			// do the transaction
			withdrawn[tid] += amount;	// success, sum up total
		else	// not enough resources left
			if (verbose > 2)
				fprintf(stderr, "thread %d: Transaction rejected: %ld, %ld\n", tid,
						cs_method == CS_METHOD_ATOMIC ? balance_atomic : balance, -amount);

		cs_leave(tid);					// critical section end
	}

	if (verbose > 1)
		printf("Thread %2d: transactions performed: %9ld\n", tid, i);

	return NULL;
	#undef tid
}

int main(int argc, char *argv[])
{
	pthread_t tids[MAX_THREADS];
	int t[MAX_THREADS];
	int i;
	long initial_amount;
	long total_withdrawn = 0;

	// argument(s) evaluation
	eval_args(argc, argv);

	balance_atomic = balance = initial_amount = thread_count * per_thread;

	if (cs_method != CS_METHOD_ATOMIC && (cs_method < CS_METHOD_MIN || cs_method > CS_METHOD_MAX)) {
		fprintf(stderr, "No valid CS method specified.\n");
		return 2;
	}

	// report initial state
	if (verbose)
		printf("%-20s %9ld\n", "The initial balance:", balance);

	// release used resources automatically upon exit
	atexit(release_resources);
 
	// init for the critical section access control; failure to init = exit
	cs_init(cs_method);
 
 	// barrier initialization
 	if (do_sync_start) {
		if ((errno = pthread_barrier_init(&sync_start_barrier, NULL, thread_count))) {
 			perror("pthread_barrier_init");
 			return 3;
 		}
		sync_start_barrier_initialized = true;
 	}
	else
		time_init();
 
	// create threads
	for (i = 0; i < thread_count; ++i) {
		t[i] = i;
		if ((errno = pthread_create(&tids[i], NULL, do_withdrawals, &t[i]))) {
			perror("pthread_create");
			return EXIT_FAILURE;
		}
	}

	if (verbose)
		printf("Threads started: %d\n", i);

	// wait for the threads termination
	for (i = 0; i < thread_count; ++i)
		if ((errno = pthread_join(tids[i], NULL))) {
			perror("pthread_join");
			return EXIT_FAILURE;
		}

	// calculate the CPU and real time used by threads
	getrusage(RUSAGE_SELF, &CPU_time2);
	clock_gettime(CLOCK_MONOTONIC, &real_time2);

	// substract init time, merge the values (seconds and microseconds)
	real_time = (double) (real_time2.tv_sec - real_time1.tv_sec) + (double) (real_time2.tv_nsec - real_time1.tv_nsec) / 1000000000.0;
	CPU_time_user =
	    (double) (CPU_time2.ru_utime.tv_sec - CPU_time1.ru_utime.tv_sec) + (double) (CPU_time2.ru_utime.tv_usec - CPU_time1.ru_utime.tv_usec) / 1000000.0;
	CPU_time_system =
	    (double) (CPU_time2.ru_stime.tv_sec - CPU_time1.ru_stime.tv_sec) + (double) (CPU_time2.ru_stime.tv_usec - CPU_time1.ru_stime.tv_usec) / 1000000.0;

	// print the used time
	printf("The time spent on the CPU(s) in milliseconds (real user system): "
	       "%.0lf %.0lf %.0lf\n", real_time * 1000, CPU_time_user * 1000, CPU_time_system * 1000);

	for (i = 0; i < thread_count; ++i) {
		// sum up the total withdrawn amount by each thread
		total_withdrawn += withdrawn[i];
		if (verbose)
			printf("%2d %-17s %9ld\n", i, "thread withdrawn:", withdrawn[i]);
	}

	if (cs_method == CS_METHOD_ATOMIC)	// atomic type was used, update normal
		balance = balance_atomic;

	// report the total amount withdrawn and the new state
	if (verbose) {
		printf("%-20s %9ld\n", "The new balance:", balance);
		printf("%-20s %9ld\n", "Total withdrawn:", total_withdrawn);
	}

	// check the result and report
	if (balance != initial_amount - total_withdrawn) {
		fprintf(stderr, "LOST TRANSACTIONS DETECTED!\n"
				"initial − new != total withdrawn (%ld != %ld)\n",
				initial_amount - balance, total_withdrawn);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

// usage
void usage(FILE * stream, char *self)
{
	fprintf(stream,
		"Usage:\n"
		"  %s -h\n"
		"  %s [-q|-v] -m method [-y] [-c threads] [-t tansactions]\n"
		"Purpose:\n"
		"  Simulation of concurrent bank transactions.\n"
		"Options:\n"
		"  -h	help\n"
		"  -m #	the method used for critical section access control (see below)\n"
		"  -y 	use sched_yield(2) during busy wait (default no)\n"
		"  -c #	the number of concurrent threads (%u, max. %d)\n"
		"  -t #	the number of transactions per one thread (%lu)\n"
		"  -q	do not print account balance state\n"
		"  -v	print more verbose information\n"
		"Methods available:\n"
		"  %2d	use atomic type\n"
		"  %2d	SW with the locked variable\n"
		"  %2d	HW with the test-and-set (xchg) instruction\n"
		"  %2d	HW with SW test and the test-and-set (xchg) instruction\n"
		"  %2d	POSIX mutex\n"
		"  %2d	POSIX unnamed semaphore\n"
		"  %2d	POSIX named semaphore\n"
		"  %2d	System V semaphore\n"
		"  %2d	POSIX message queue\n"
		"  %2d	System V message queue\n"
		, self, self
		, thread_count, MAX_THREADS
		, per_thread
		, CS_METHOD_ATOMIC
		, CS_METHOD_LOCKED
		, CS_METHOD_XCHG
		, CS_METHOD_TEST_XCHG
		, CS_METHOD_MUTEX
		, CS_METHOD_SEM_POSIX
		, CS_METHOD_SEM_POSIX_NAMED
		, CS_METHOD_SEM_SYSV
		, CS_METHOD_MQ_POSIX
		, CS_METHOD_MQ_SYSV
		);
}

// arguments and switches evaluation
void eval_args(int argc, char *argv[])
{
	int opt;		// option

	opterr = 0;		// do not print errors, we'll print it
	// switches processing
	while (-1 != (opt = getopt(argc, argv, "hwqvc:t:a:f:s:m:y"))) {
		switch (opt) {
		// -c thread_count
		case 'c':
			thread_count = strtol(optarg, NULL, 0);
			if (thread_count < 1 || thread_count > MAX_THREADS) {
				fprintf(stderr, "The number of threads is limited to 1 upto %d\n", MAX_THREADS);
				exit(2);
			}
			break;
		// -t transactions_per_thread
		case 't':
			per_thread = strtol(optarg, NULL, 0);
			break;
		// quiet
		case 'q':
			verbose = 0;
			break;
		// more verbose
		case 'v':
			verbose++;
			break;
		// wait for synchronous start
		case 'w':
			do_sync_start = true;
			break;
		// -m method
		case 'm':
			cs_method = strtol(optarg, NULL, 0);
			break;
		// use sched_yield(2) during busy waits
		case 'y':
			busy_wait_yields = true;
			break;
		// help
		case 'h':
			usage(stdout, argv[0]);
			exit(EXIT_SUCCESS);
			break;
		// unknown option
		default:
			fprintf(stderr, "%c: unknown option.\n", optopt);
			usage(stderr, argv[0]);
			exit(2);
			break;
		}
	}
}

// vim:ts=4:sw=4
