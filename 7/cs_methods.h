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
#include <sys/ipc.h>					// IPC_PRIVATE, IPC_RMID
#include <sys/msg.h>					// System V message queue
#include <errno.h>						// perror

bool busy_wait_yields = false;			// set by the main program

// macros, variable declarations and function definitions for critical section access control
struct msgbuf {							// CS_METHOD_MQ_SYSV
	long int msg_type;
};
int mq_sys_v_locked;					// CS_METHOD_MQ_SYSV
struct msgbuf mq_sys_v_msg;				// CS_METHOD_MQ_SYSV

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
		break;
	case CS_METHOD_MQ_POSIX:
		break;
	case CS_METHOD_MQ_SYSV:
		if ((mq_sys_v_locked = msgget(IPC_PRIVATE, 0600)) == -1) {
										// svmq = System V message queue
										// key - IPC_PRIVATE: to create private svmq for process
										// msgflg - 0600: rw for process owner
			perror("msgget");
			exit(EXIT_FAILURE);
		}
		mq_sys_v_msg.msg_type = 1;		// msg_type must be > 0, msg_text not required
		if (msgsnd(mq_sys_v_locked, (void *) &mq_sys_v_msg, 0, 0) == -1) {
										// msgsz - 0: message size is not needed
										// msgflg - 0: irrelevant, queue will never fully fill
			perror("msgsnd init");
			exit(EXIT_FAILURE);
		}
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
		break;
	case CS_METHOD_MQ_POSIX:
		break;
	case CS_METHOD_MQ_SYSV:
		if (msgctl(mq_sys_v_locked, IPC_RMID, NULL) == -1) {
										// immediately remove svmq awakening all waiting reader and writer processes
										// cmd - IPC_RMID: provides functionality above
										// buf - NULL: msqid_ds not used
			perror("msgctl destroy");
		}
		break;
	}
	cs_var_allocated = false;
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
		break;
	case CS_METHOD_MQ_POSIX:
		break;
	case CS_METHOD_MQ_SYSV:
		if (msgrcv(mq_sys_v_locked, (void *) &mq_sys_v_msg, 0, 0, 0) == -1) {
										// msgsz - 0: message size is not needed
										// msgtyp - 0: first message in queue shall be received
										// msgflg - 0: when no message is present, wait/block thread
			perror("msgrcv");
			exit(EXIT_FAILURE);
		}
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
		break;
	case CS_METHOD_MQ_POSIX:
		break;
	case CS_METHOD_MQ_SYSV:
		if (msgsnd(mq_sys_v_locked, (void *) &mq_sys_v_msg, 0, 0) == -1) {
										// msgsz - 0: message size is not needed
										// msgflg - 0: irrelevant, queue will never fully fill
			perror("msgsnd");
			exit(EXIT_FAILURE);
		}
		break;
	}
}

// vim:ts=4:sw=4
// EOF
