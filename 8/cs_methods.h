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
#include <stdatomic.h>					// atomic_flag
#include <sched.h>						// sched_yield(2)
#include <pthread.h>					// POSIX mutex
#include <semaphore.h>					// POSIX semaphores
#include <mqueue.h>						// POSIX message queue
#include <fcntl.h>						// open flags: O_CREAT, O_RDWR
#include <sys/stat.h>					// permissions: S_IRUSR, S_IWUSR
#include <sys/ipc.h>					// IPC_PRIVATE, IPC_RMID
#include <sys/sem.h>					// System V semaphores
#include <sys/msg.h>					// System V message queue
#include <errno.h>						// errno, perror

bool busy_wait_yields = false;			// set by the main program

// macros, variable declarations and function definitions for critical section access control
volatile bool locked;									// CS_METHOD_LOCKED, CS_METHOD_TEST_XCHG
volatile atomic_flag xchg_locked;						// CS_METHOD_XCHG
pthread_mutex_t mutex_locked;							// CS_METHOD_MUTEX
sem_t sem_locked;										// CS_METHOD_SEM_POSIX
#define SEM_NAME "/cs_methods-sem-st58214"				// CS_METHOD_SEM_POSIX_NAMED
sem_t *psem_named_locked;								// CS_METHOD_SEM_POSIX_NAMED
int sem_sys_v_locked;									// CS_METHOD_SEM_SYSV
struct sembuf sops_wait;								// CS_METHOD_SEM_SYSV
struct sembuf sops_post;								// CS_METHOD_SEM_SYSV
#define MQ_POSIX_NAME "/cs_methods-posix_mq-st58214"	// CS_METHOD_MQ_POSIX
#define MQ_POSIX_MESSAGE "lock"							// CS_METHOD_MQ_POSIX
#define MQ_POSIX_MESSAGE_LIMIT 4						// CS_METHOD_MQ_POSIX
mqd_t mq_posix_locked;									// CS_METHOD_MQ_POSIX
struct mq_attr mq_posix_attr;							// CS_METHOD_MQ_POSIX
char mq_posix_buffer[MQ_POSIX_MESSAGE_LIMIT + 1];		// CS_METHOD_MQ_POSIX
struct msgbuf {											// CS_METHOD_MQ_SYSV
	long int msg_type;
};
int mq_sys_v_locked;									// CS_METHOD_MQ_SYSV
struct msgbuf mq_sys_v_msg;								// CS_METHOD_MQ_SYSV


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
		return;
	case CS_METHOD_LOCKED:
	case CS_METHOD_TEST_XCHG:
		locked = false;
		return;
	case CS_METHOD_XCHG:
		atomic_flag_clear(&xchg_locked);					// sets atomic_flag object to false
		return;
	case CS_METHOD_MUTEX:
		if ((errno = pthread_mutex_init(&mutex_locked, NULL))) {
															// initialize POSIX mutex
															// attr - NULL: no need for attributes
			perror("CS_METHOD_MUTEX: pthread_mutex_init");
			exit(EXIT_FAILURE);
		}
		break;
	case CS_METHOD_SEM_POSIX:
		if (sem_init(&sem_locked, 0, 1) == -1) {			// initialize POSIX semaphore
															// pshared - 0: semaphore sharing between threads
															// value - 1: initialize semaphore counter to 1
			perror("CS_METHOD_SEM_POSIX: sem_init");
			exit(EXIT_FAILURE);
		}
		break;
	case CS_METHOD_SEM_POSIX_NAMED:
		if ((psem_named_locked = sem_open(SEM_NAME, O_CREAT|O_EXCL, S_IRUSR|S_IWUSR, 1)) == SEM_FAILED) {
															// create and open new named POSIX semaphore
															// SEM_NAME - name of created semaphore
															// O_CREAT: oflag to create semaphore if it does not exist
															// O_EXCL: oflag to fail if semaphore with same name already exists
															// mode - S_IRUSR: read right, S_IWUSR: write right
															// value - 1: initialize semaphore counter to 1
			perror("CS_METHOD_SEM_POSIX_NAMED: sem_open");
			exit(EXIT_FAILURE);
		}

		if (sem_unlink(SEM_NAME) == -1) {					// removes the semaphore name immediately
															// named semaphore is destroyed after all other processes close it
			perror("CS_METHOD_SEM_POSIX_NAMED: sem_unlink");
			exit(EXIT_FAILURE);
		}
		break;
	case CS_METHOD_SEM_SYSV:
		if ((sem_sys_v_locked = semget(IPC_PRIVATE, 1, 0600)) == -1) {
															// key - IPC_PRIVATE: to create private semaphore set for process
															// nsems - 1: number of created semaphores in set
															// semflg - 0600: rw for process owner
			perror("CS_METHOD_SEM_SYSV: semget");
			exit(EXIT_FAILURE);
		}

		if (semctl(sem_sys_v_locked, 0, SETVAL, 1) == -1) {	// init first semaphore to 1 using command SETVAL
			perror("CS_METHOD_SEM_SYSV: semctl init");
			exit(EXIT_FAILURE);
		}

		sops_wait.sem_num = 0;								// semaphore number - 0: because we have only one semaphore
		sops_wait.sem_flg = SEM_UNDO;						// operation flag - SEM_UNDO: revert on process failure
		sops_wait.sem_op = -1;								// semaphore operation - (-1): wait
		sops_post.sem_num = 0;
		sops_post.sem_flg = SEM_UNDO;
		sops_post.sem_op = 1;								// semaphore operation - (1): post
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
			perror("CS_METHOD_MQ_POSIX: mq_open");
			exit(EXIT_FAILURE);
		}

		if (mq_unlink(MQ_POSIX_NAME) == -1) {				// removes mq name immediately
															// mq is destroyed after all other processes close it
			perror("CS_METHOD_MQ_POSIX: mq_unlink");
			exit(EXIT_FAILURE);
		}

		if (mq_send(mq_posix_locked, MQ_POSIX_MESSAGE, MQ_POSIX_MESSAGE_LIMIT, 0) == -1) {
															// send message to mq to set curmsgs to 1
															// msg_prio - 0: priority value, needed but not used
            perror("CS_METHOD_MQ_POSIX: mq_send init");
            exit(EXIT_FAILURE);
        }
		break;
	case CS_METHOD_MQ_SYSV:
		if ((mq_sys_v_locked = msgget(IPC_PRIVATE, 0600)) == -1) {
															// svmq = System V message queue
															// key - IPC_PRIVATE: to create private svmq for process
															// msgflg - 0600: rw for process owner
			perror("CS_METHOD_MQ_SYSV: msgget");
			exit(EXIT_FAILURE);
		}

		mq_sys_v_msg.msg_type = 1;							// msg_type must be > 0, msg_text not required

		if (msgsnd(mq_sys_v_locked, (void *) &mq_sys_v_msg, 0, 0) == -1) {
															// msgsz - 0: message size is not needed
															// msgflg - 0: irrelevant, queue will never fully fill
			perror("CS_METHOD_MQ_SYSV: msgsnd init");
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
		if ((errno = pthread_mutex_destroy(&mutex_locked))) {
											// destroy mutex
			perror("CS_METHOD_MUTEX: pthread_mutex_destroy");
		}
		break;
	case CS_METHOD_SEM_POSIX:
		if (sem_destroy(&sem_locked) == -1) {
											// destroy semaphore
			perror("CS_METHOD_SEM_POSIX: sem_destroy");
		}
		break;
	case CS_METHOD_SEM_POSIX_NAMED:
		if (sem_close(psem_named_locked) == -1) {
											// destroy named semaphore
			perror("CS_METHOD_SEM_POSIX_NAMED: sem_close");
		}
		psem_named_locked = SEM_FAILED;		// drop link to semaphore so it can be deleted
											// should be already deleted after sem_close when using sem_unlink
		break;
	case CS_METHOD_SEM_SYSV:
		if (semctl(sem_sys_v_locked, 0, IPC_RMID) == -1) {
											// immediately remove semaphore set awakening all blocked processes
											// semnum - 0: index of semaphore
											// cmd - IPC_RMID: provides functionality above
			perror("CS_METHOD_SEM_SYSV: semctl destroy");
		}
		break;
	case CS_METHOD_MQ_POSIX:
		if (mq_close(mq_posix_locked) == -1) {
											// destroy mq
			perror("CS_METHOD_MQ_POSIX: mq_close");
		}
		break;
	case CS_METHOD_MQ_SYSV:
		if (msgctl(mq_sys_v_locked, IPC_RMID, NULL) == -1) {
											// immediately remove svmq awakening all waiting reader and writer processes
											// cmd - IPC_RMID: provides functionality above
											// buf - NULL: msqid_ds not used
			perror("CS_METHOD_MQ_SYSV: msgctl destroy");
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
		while (locked) {
			if (busy_wait_yields) {
				sched_yield();		// causes the calling thread to relinquish the CPU, thread is moved to the end of queue
			}
		}
		locked = true;
		break;
	case CS_METHOD_TEST_XCHG:
		while (atomic_load_explicit(&locked, memory_order_relaxed) || atomic_exchange_explicit(&locked, true, memory_order_acquire)) {
									// atomically check if locked
									// desired - true: value to atomically exchange with
									// memory_order_relaxed: guaratees only atomicity of operation
									// memory_order_acquire: ensures sequential consistency of atomics across threads
			if (busy_wait_yields) {
				sched_yield();		// causes the calling thread to relinquish the CPU, thread is moved to the end of queue
			}
		}
		break;
	case CS_METHOD_XCHG:
		while (atomic_flag_test_and_set(&xchg_locked)) {	// request lock using atomic operation
			if (busy_wait_yields) {
				sched_yield();		// causes the calling thread to relinquish the CPU, thread is moved to the end of queue
			}
		}
		break;
	case CS_METHOD_MUTEX:
		errno = pthread_mutex_lock(&mutex_locked);		// no error checking due to performance testing
														// try to lock mutex
		break;
	case CS_METHOD_SEM_POSIX:
		sem_wait(&sem_locked);							// no error checking due to performance testing
														// wait on semaphore
		break;
	case CS_METHOD_SEM_POSIX_NAMED:
		sem_wait(psem_named_locked);					// no error checking due to performance testing
														// wait on named semaphore
		break;
	case CS_METHOD_SEM_SYSV:
		semop(sem_sys_v_locked, &sops_wait, 1);			// no error checking due to performance testing
														// wait on System V semaphore
														// 1 to represent number of affected semaphores
		break;
	case CS_METHOD_MQ_POSIX:
		mq_receive(mq_posix_locked, mq_posix_buffer, MQ_POSIX_MESSAGE_LIMIT, NULL);
														// no error checking due to performance testing
														// receive message from mq to act as wait
														// *msg_prio - NULL: no need for priority
		break;
	case CS_METHOD_MQ_SYSV:
		msgrcv(mq_sys_v_locked, (void *) &mq_sys_v_msg, 0, 0, 0);
														// no error checking due to performance testing
														// msgsz - 0: message size is not needed
														// msgtyp - 0: first message in queue shall be received
														// msgflg - 0: when no message is present, wait/block thread
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
		locked = false;
		break;
	case CS_METHOD_TEST_XCHG:
		atomic_store_explicit(&locked, false, memory_order_release);
									// desired - false: value to atomically exchange with
									// memory_order_release: ensures sequential consistency of atomics across threads
		break;
	case CS_METHOD_XCHG:
		atomic_flag_clear(&xchg_locked);						// sets atomic_flag object to false
		break;
	case CS_METHOD_MUTEX:
		errno = pthread_mutex_unlock(&mutex_locked);			// no error checking due to performance testing
																// unlock mutex
		break;
	case CS_METHOD_SEM_POSIX:
		sem_post(&sem_locked);									// no error checking due to performance testing
																// post on semaphore
		break;
	case CS_METHOD_SEM_POSIX_NAMED:
		sem_post(psem_named_locked);							// no error checking due to performance testing
																// post on named semaphore
		break;
	case CS_METHOD_SEM_SYSV:
		semop(sem_sys_v_locked, &sops_post, 1);					// no error checking due to performance testing
																// post on System V semaphore
																// 1 to represent number of affected semaphores
		break;
	case CS_METHOD_MQ_POSIX:
		mq_send(mq_posix_locked, MQ_POSIX_MESSAGE, MQ_POSIX_MESSAGE_LIMIT, 0);
																// no error checking due to performance testing
																// send message to mq to act as post
																// msg_prio - 0: priority value, needed but not used
		break;
	case CS_METHOD_MQ_SYSV:
		msgsnd(mq_sys_v_locked, (void *) &mq_sys_v_msg, 0, 0);
																// no error checking due to performance testing
																// msgsz - 0: message size is not needed
																// msgflg - 0: irrelevant, queue will never fully fill
		break;
	}
}

// vim:ts=4:sw=4
// EOF
