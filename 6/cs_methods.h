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
#include <fcntl.h>						// open flags: O_CREAT, O_RDWR, O_EXCL
#include <sys/stat.h>					// permissions: S_IRUSR, S_IWUSR
#include <mqueue.h>						// POSIX message queue
#include <errno.h>						// perror

bool busy_wait_yields = false;			// set by the main program

// macros, variable declarations and function definitions for critical section access control
#define MQ_POSIX_NAME "/cs_methods-posix_mq-st58214"	// CS_METHOD_MQ_POSIX
#define MQ_POSIX_MESSAGE ""								// CS_METHOD_MQ_POSIX
#define MQ_POSIX_MESSAGE_LIMIT 1						// CS_METHOD_MQ_POSIX
mqd_t mq_posix_locked;									// CS_METHOD_MQ_POSIX
struct mq_attr mq_posix_attr;							// CS_METHOD_MQ_POSIX
char mq_posix_buffer[MQ_POSIX_MESSAGE_LIMIT];			// CS_METHOD_MQ_POSIX

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
															// mq = POSIX message queue
		mq_posix_attr.mq_flags = 0;							// no need for O_NONBLOCK
		mq_posix_attr.mq_maxmsg = 1;						// maximum number of messages in the queue
		mq_posix_attr.mq_msgsize = MQ_POSIX_MESSAGE_LIMIT;	// maximum message size
		mq_posix_attr.mq_curmsgs = 0;						// number of current messages in queue, left default

		if ((mq_posix_locked = mq_open(MQ_POSIX_NAME, O_CREAT|O_EXCL|O_RDWR, S_IRUSR|S_IWUSR, &mq_posix_attr)) == (mqd_t) -1) {
															// O_CREAT: oflag to create mq if it does not exist
															// O_EXCL: oflag to fail if mq with same name already exists
															// O_RDWR: oflag to open mq for receive and send
															// mode - S_IRUSR: read right, S_IWUSR: write right
			perror("mq_open");
			exit(EXIT_FAILURE);
		}

		if (mq_unlink(MQ_POSIX_NAME) == -1) {
															// removes mq name immediately
															// mq is destroyed after all other processes close it
			perror("mq_unlink");
			exit(EXIT_FAILURE);
		}

		if (mq_send(mq_posix_locked, MQ_POSIX_MESSAGE, MQ_POSIX_MESSAGE_LIMIT, 0) == -1) {
															// send message to mq to set curmsgs to 1
															// msg_prio - 0: priority value, needed but not used
            perror("mq_send init");
            exit(EXIT_FAILURE);
        }
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
		break;
	case CS_METHOD_MQ_POSIX:
		if (mq_close(mq_posix_locked) == -1) {				// destroy mq
			perror("mq_close");
		}
		break;
	case CS_METHOD_MQ_SYSV:
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
		if (mq_receive(mq_posix_locked, mq_posix_buffer, MQ_POSIX_MESSAGE_LIMIT, NULL) == -1) {
															// receive message from mq to act as wait
															// *msg_prio - NULL: no need for priority
            perror("mq_receive");
            exit(EXIT_FAILURE);
        }
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
		break;
	case CS_METHOD_MQ_POSIX:
		if (mq_send(mq_posix_locked, MQ_POSIX_MESSAGE, MQ_POSIX_MESSAGE_LIMIT, 0) == -1) {
															// send message to mq to act as post
															// msg_prio - 0: priority value, needed but not used
            perror("mq_send");
            exit(EXIT_FAILURE);
        }
		break;
	case CS_METHOD_MQ_SYSV:
		break;
	}
}

// vim:ts=4:sw=4
// EOF
