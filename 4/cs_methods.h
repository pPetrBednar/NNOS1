// Operating Systems: sample code  (c) Tomáš Hudec
// Critical Section Access Control
// header file

// Modified: 2017-11-30, 2017-12-06, 2020-11-25, 2020-12-10, 2023-11-23

#define CS_METHOD_ATOMIC				0
#define CS_METHOD_LOCKED				1
#define CS_METHOD_XCHG					2
#define CS_METHOD_TEST_XCHG				3
#define CS_METHOD_MUTEX					4
#define CS_METHOD_SEM_POSIX				5
#define CS_METHOD_SEM_POSIX_NAMED		6
#define CS_METHOD_SEM_SYSV				7
#define CS_METHOD_MQ_POSIX				8
#define CS_METHOD_MQ_SYSV				9

#define CS_METHOD_MIN					CS_METHOD_LOCKED
#define CS_METHOD_MAX					CS_METHOD_MQ_SYSV
#define CS_METHODS_BUSY_WAIT			3

#include <stdbool.h>					// bool
#include <stdlib.h>						// exit
#include <stdio.h>						// fprintf
// additional includes for critical section access control methods
#include <pthread.h>					// POSIX mutex
#include <semaphore.h>					// POSIX semaphores
#include <fcntl.h>						// open flags: O_CREAT
#include <sys/stat.h>					// permissions: S_IRUSR, S_IWUSR
#include <errno.h>						// errno

bool busy_wait_yields = false;			// set by the main program

// macros, variable declarations and function definitions for critical section access control
pthread_mutex_t mutex_locked;			// CS_METHOD_MUTEX
sem_t sem_locked;						// CS_METHOD_SEM_POSIX
#define SEM_NAME "/cs_methods-sem"		// CS_METHOD_SEM_POSIX_NAMED
sem_t *psem_named_locked;				// CS_METHOD_SEM_POSIX_NAMED

// note: inline is not used unless asked for optimization
#define FORCE_INLINE	__attribute__ ((always_inline)) static inline

// allocate/initialize variables used for the critical section access control
FORCE_INLINE
void cs_init(int method);

// destroy allocated variables used for the critical section access control
FORCE_INLINE
void cs_destroy(void);

// before entering the critical section
FORCE_INLINE
void cs_enter(int id);

// after leaving the critical section
FORCE_INLINE
void cs_leave(int id);


static int cs_method_used = -1;			// method used, initialized in cs_init()
static bool cs_var_allocated = false;	// successful allocation of variables


// implementation (the funcions are to be inlined, we need them here)


// allocate/initialize variables used for the critical section access control
void cs_init(int method)
{
	cs_method_used = method;
	switch (cs_method_used) {
	case CS_METHOD_ATOMIC:
		break;
	case CS_METHOD_LOCKED:
	case CS_METHOD_TEST_XCHG:
		break;
	case CS_METHOD_XCHG:
		break;
	case CS_METHOD_MUTEX:
		if ((errno = pthread_mutex_init(&mutex_locked, NULL))) {
										// initialize POSIX mutex
										// attr - NULL: no need for attributes
			perror("pthread_mutex_init");
			exit(EXIT_FAILURE);
		}
		break;
	case CS_METHOD_SEM_POSIX:
		if (sem_init(&sem_locked, 0, 1) == -1) {
										// initialize POSIX semaphore
										// pshared - 0: semaphore sharing between threads
										// value - 1: initialize semaphore counter to 1
			perror("sem_init");
			exit(EXIT_FAILURE);
		}
		break;
	case CS_METHOD_SEM_POSIX_NAMED:
		if ((psem_named_locked = sem_open(SEM_NAME, O_CREAT, S_IRUSR|S_IWUSR, 1)) == SEM_FAILED) {
										// create and open new named POSIX semaphore
										// SEM_NAME - name of created semaphore
										// oflag - O_CREAT: semaphore is created if it does not exist
										// mode - S_IRUSR: read right, S_IWUSR: write right
										// value - 1: initialize semaphore counter to 1
			perror("sem_open");
			exit(EXIT_FAILURE);
		}
		if (sem_unlink(SEM_NAME) == -1) {
										// removes the semaphore name immediately
										// named semaphore is destroyed after all other processes close it
			perror("sem_unlink");
			exit(EXIT_FAILURE);
		}
		break;
	case CS_METHOD_SEM_SYSV:
		break;
	case CS_METHOD_MQ_POSIX:
		break;
	case CS_METHOD_MQ_SYSV:
		break;
	default:
		fprintf(stderr, "Error: The method %d is not defined.\n", cs_method_used);
		exit(EXIT_FAILURE);
	}
	cs_var_allocated = true;
}

// destroy allocated variables used for the critical section access control
void cs_destroy(void)
{
	if (!cs_var_allocated)				// if not allocated: nothing to do
		return;
	switch (cs_method_used) {
	case CS_METHOD_ATOMIC:
	case CS_METHOD_LOCKED:
	case CS_METHOD_TEST_XCHG:
	case CS_METHOD_XCHG:
		break;
	case CS_METHOD_MUTEX:
		if ((errno = pthread_mutex_destroy(&mutex_locked))) {	// destroy mutex
			perror("pthread_mutex_destroy");
			exit(EXIT_FAILURE);
		}
		break;
	case CS_METHOD_SEM_POSIX:
		if (sem_destroy(&sem_locked) == -1) {					// destroy semaphore
			perror("sem_destroy");
			exit(EXIT_FAILURE);
		}
		break;
	case CS_METHOD_SEM_POSIX_NAMED:
		if (sem_close(psem_named_locked) == -1) {				// destroy named semaphore
			perror("sem_close");
			exit(EXIT_FAILURE);
		}
		psem_named_locked = SEM_FAILED;		// drop link to semaphore so it can be deleted
											// should be already deleted after sem_close when using sem_unlink
		break;
	case CS_METHOD_SEM_SYSV:
		break;
	case CS_METHOD_MQ_POSIX:
		break;
	case CS_METHOD_MQ_SYSV:
		break;
	}
}

// before entering the critical section
void cs_enter(int id)
{
	switch (cs_method_used) {
	case CS_METHOD_ATOMIC:
		break;
	case CS_METHOD_LOCKED:
		break;
	case CS_METHOD_TEST_XCHG:
		break;
	case CS_METHOD_XCHG:
		break;
	case CS_METHOD_MUTEX:
		if ((errno = pthread_mutex_lock(&mutex_locked))) {	// try to lock mutex
			perror("pthread_mutex_lock");
			exit(EXIT_FAILURE);
		}
		break;
	case CS_METHOD_SEM_POSIX:
		if (sem_wait(&sem_locked) == -1) {					// wait on semaphore
			perror("sem_wait");
			exit(EXIT_FAILURE);
		}
		break;
	case CS_METHOD_SEM_POSIX_NAMED:
		if (sem_wait(psem_named_locked) == -1) {			// wait on named semaphore
			perror("sem_wait named");
			exit(EXIT_FAILURE);
		}
		break;
	case CS_METHOD_SEM_SYSV:
		break;
	case CS_METHOD_MQ_POSIX:
		break;
	case CS_METHOD_MQ_SYSV:
		break;
	}
}

// after leaving the critical section
void cs_leave(int id)
{
	switch (cs_method_used) {
	case CS_METHOD_ATOMIC:
		break;
	case CS_METHOD_LOCKED:
		break;
	case CS_METHOD_TEST_XCHG:
		break;
	case CS_METHOD_XCHG:
		break;
	case CS_METHOD_MUTEX:
		if ((errno = pthread_mutex_unlock(&mutex_locked))) {	// unlock mutex
			perror("pthread_mutex_unlock");
			exit(EXIT_FAILURE);
		}
		break;
	case CS_METHOD_SEM_POSIX:
		if (sem_post(&sem_locked) == -1) {						// post on semaphore
			perror("sem_post");
			exit(EXIT_FAILURE);
		}
		break;
	case CS_METHOD_SEM_POSIX_NAMED:
		if (sem_post(psem_named_locked) == -1) {				// post on named semaphore
			perror("sem_post named");
			exit(EXIT_FAILURE);
		}
		break;
	case CS_METHOD_SEM_SYSV:
		break;
	case CS_METHOD_MQ_POSIX:
		break;
	case CS_METHOD_MQ_SYSV:
		break;
	}
}

// vim:ts=4:sw=4
// EOF
