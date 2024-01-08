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
#include <sys/ipc.h>					// IPC_PRIVATE
#include <sys/sem.h>					// System V semaphores
#include <errno.h>						// perror

bool busy_wait_yields = false;			// set by the main program

// macros, variable declarations and function definitions for critical section access control
int sem_id;								// CS_METHOD_SEM_SYSV
struct sembuf sops_wait;				// CS_METHOD_SEM_SYSV
struct sembuf sops_post;				// CS_METHOD_SEM_SYSV

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
		break;
	case CS_METHOD_SEM_POSIX:
		break;
	case CS_METHOD_SEM_POSIX_NAMED:
		break;
	case CS_METHOD_SEM_SYSV:
		if ((sem_id = semget(IPC_PRIVATE, 1, 0600)) == -1) {
													// key - IPC_PRIVATE: to create private semaphore set for process
													// nsems - 1: number of created semaphores in set
													// semflg - 0600: rw for process owner
			perror("semget");
			exit(EXIT_FAILURE);
		}
		if (semctl(sem_id, 0, SETVAL, 1) == -1) { 	// init first semaphore to 1 using command SETVAL
			perror("semctl init");
			exit(EXIT_FAILURE);
		}
		sops_wait.sem_num = 0;						// semaphore number - 0: because we have only one semaphore
		sops_wait.sem_flg = SEM_UNDO;				// operation flag - SEM_UNDO: revert on process failure
		sops_wait.sem_op = -1;						// semaphore operation - (-1): wait
		sops_post.sem_num = 0;
		sops_post.sem_flg = SEM_UNDO;
		sops_post.sem_op = 1;						// semaphore operation - (1): post
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
		break;
	case CS_METHOD_SEM_POSIX:
		break;
	case CS_METHOD_SEM_POSIX_NAMED:
		break;
	case CS_METHOD_SEM_SYSV:
		if (semctl(sem_id, 0, IPC_RMID) == -1) {
										// immediately remove semaphore set awakening all blocked processes
										// cmd - IPC_RMID: provides functionality above
			perror("semctl destroy");
			exit(EXIT_FAILURE);
		}
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
		break;
	case CS_METHOD_SEM_POSIX:
		break;
	case CS_METHOD_SEM_POSIX_NAMED:
		break;
	case CS_METHOD_SEM_SYSV:
		if (semop(sem_id, &sops_wait, 1) == -1) {
										// wait on System V semaphore
										// 1 to represent number of affected semaphores
			perror("semop wait");
			exit(EXIT_FAILURE);
		}
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
		break;
	case CS_METHOD_SEM_POSIX:
		break;
	case CS_METHOD_SEM_POSIX_NAMED:
		break;
	case CS_METHOD_SEM_SYSV:
		if (semop(sem_id, &sops_post, 1) == -1) {
										// post on System V semaphore
										// 1 to represent number of affected semaphores
			perror("semop post");
			exit(EXIT_FAILURE);
		}
		break;
	case CS_METHOD_MQ_POSIX:
		break;
	case CS_METHOD_MQ_SYSV:
		break;
	}
}

// vim:ts=4:sw=4
// EOF
