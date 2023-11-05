// Operating Systems: sample code  (c) Tomáš Hudec
// POSIX Signals, signal set operations
// sigaction(2), alarm(2), sigsuspend(2), sigsetops(3)

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>

bool interrupted = false;			// was interrupted?

void catch_sig(int signo) {
	printf("\nCatched the signal:");
	switch (signo) {
		case SIGINT:
			alarm(0);		// cancel the alarm
			printf(" SIGINT");
			interrupted = true;	// set interrupted to true
			break;
		case SIGALRM:
			printf(" SIGALRM");
	}
	printf(" %d\n", signo);
}

int main(int argc, char *argv[]) {
	sigset_t sigs;
	struct sigaction sa_old;
	struct sigaction sa_new;

	memset(&sa_new, 0, sizeof(sa_new));	// initialization to zeros
	sa_new.sa_handler = catch_sig;		// set handler
	sigemptyset(&sa_new.sa_mask);		// mask: empty set
	sigaddset(&sa_new.sa_mask, SIGALRM);	// mask: add SIGALRM
	sigaddset(&sa_new.sa_mask, SIGINT);	// mask: add SIGINT
	sa_new.sa_flags = 0;			// no flags

	sigaction(SIGINT, &sa_new, &sa_old);	// set the handler for SIGINT and get old handler setting
	sigaction(SIGALRM, &sa_new, NULL);	// set the same handler for SIGALRM

	sigfillset(&sigs);			// all signals to the set
	sigdelset(&sigs, SIGINT);		// remove SIGINT from the set
	sigdelset(&sigs, SIGALRM);		// remove SIGALRM from the set

	puts("You have 5 seconds to interrupt by SIGINT (press CTRL+C to break).");

	alarm(5);		// timeout after 5 seconds
	sigsuspend(&sigs);	// wait for SIGINT or SIGALRM

	if (interrupted) {
		puts("User requested to break.");
		// exit
		return 1;
	}

	puts("Process continues.");
	// do the job
	return 0;
}
