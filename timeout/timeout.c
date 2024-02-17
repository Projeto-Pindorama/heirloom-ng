/* 
 * timeout.c - execute a command with a time limit 
 */
/*
 * Copyright (C) 2023, 2024: Luiz Antônio (takusuman)
 *
 * SPDX-Licence-Identifier: Zlib
 *
 * The 'validate_duration' function contains code fro Heirloom-NG's
 * watch(1) implementation that parses a float for timespec structs.
 * This code was written by Arthur Bacci and it's covered per the
 * same terms as this program.
 *
 * The part that makes the called command, in fact, sleep was
 * throloughly based on OpenBSD's implementation of timeout.
 * As per the copyright header of OpenBSD's "usr.bin/timeout/timeout.c":
 * Copyright (c) 2021 Job Snijders <job@openbsd.org>
 * Copyright (c) 2014 Baptiste Daroussin <bapt@FreeBSD.org>
 * Copyright (c) 2014 Vsevolod Stakhov <vsevolod@FreeBSD.org>
 *
 * SPDX-Licence-Identifier: BSD-2-Clause
 */

#include <errno.h>
#include <float.h>
#include <unistd.h>
#include <signal.h>
#include <sigtable.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pfmt.h>
#include <wait.h>
#include <time.h>
#include <ctype.h>

char *getenv(const char *);

static char *progname;

int main(int argc, char *argv[]);
int validate_signal(char *signal_name);
float conv_duration(char *timestr);
struct TClock validate_duration(char *duration);
void settimeout(struct TClock *duration);
void handle_signal(int signo);
void usage(void);

struct TClock {
	time_t sec;
	long int nsec;
};

struct LSignal {
	sig_atomic_t sig_alrm;
	sig_atomic_t sig_chld;
	sig_atomic_t sig_term;
	sig_atomic_t sig_ign;
};
static struct LSignal siglist;

/*
 * Rodar o comando e bifurcar para o fundo
 * "Matar" o comando depois de X tempo com SIGTERM
 * Se ainda estiver rodando, matar em definitivo com
 * o sinal informado pelo usuário --- esperar por mais
 * tempo? Opção '-k'.
 */

int main(int argc, char *argv[]) {
	progname = argv[0];
	register int c = 0,
	    s = 0;
	int option = 0,
	    fForeground = 0,
	    fOnemoretime = 0,
	    fPreserve_status = 0,
	    killer_sig = SIGTERM, /* Default killer signal is SIGTERM. */
			/* 
			 * -1 can be replaced by another signal
			 * if the user sets the -s flag.
			 */
	    sigsused[] = { -1,  SIGHUP, SIGINT, SIGQUIT,
		    		SIGTERM, SIGCHLD, SIGALRM },
	    execerr = 0, /* Command execution error code. */
	    ecmd = 0, /* Command exit code. */
	    eprog = 0, /* Program exit code. */
	    timesout = 0;
	char **commandv;
	pid_t cmdpid = 0,
	      exec_pid = 0,
	      pgid = 0;
	
	struct TClock first_interval = {0},
	      second_interval = {0};
	struct sigaction sa = {0};

	/* Debug block. */
	FILE *doutput;

	if (getenv("HEIRLOOM_DEBUG")) {
		if ((doutput = fopen("debug.txt", "w")) == NULL) {
			pfmt(stderr, MM_ERROR, "failed to open debug.txt.\n");
		}
		exit(1);
	} else {
		doutput = fopen("/dev/null", "w");
	}

	while ( (option = getopt(argc, argv, "fps:k:h")) != -1 ){
		fprintf(doutput, "%s: getopt: Chose option '%c'.\n", progname, option);
		switch (option) {
			case 'f':
				/* According to GNU's timeout(1) usage()
				 * function:
				 * "when not running timeout directly from a
				 * shell prompt, allow COMMAND to read from
				 * the TTY and get TTY signals; in this mode,
				 * children of COMMAND will not be timed out.
				 */
				fForeground = 1;
				break;
			case 'p':
				/* Simplest option overall, it catches process'
				 * exit code and returns it instead of returning
				 * timeout's default "timed out" exit code (124).
				 * This is only valid for processes that did not
				 * exit as zero.
				 */
				fPreserve_status = 1;
				break;
			case 's':
				killer_sig = validate_signal(optarg);
				break;
			case 'k':
				fOnemoretime = 1;
				second_interval = validate_duration(optarg);
				break;
			case 'h':
			default:
				usage();
				break;
		}
	}
	argc -= optind;
	argv += optind;

	/* Missing operand. */
	if ( argc < 2 ){
		usage();
	}

	/* Allocate commandv[], where argv[] will be copied to. */
	if ( (commandv = calloc((unsigned long)(argc + 1), sizeof(char *))) == NULL ) { 
		pfmt(stderr, MM_ERROR,
				"%s: could not allocate an array of %lu elements, each one being %lu bytes large.\n",
				progname, (unsigned long)(argc + 1), (sizeof(char *))
		);
		exit(1);
	}

	/* And then copy argv[] to commandv[]. */
	for (c = 0; c < argc; c++) {
		commandv[c] = argv[c];
		fprintf(doutput, "%s: assign argv[%d] to commandv[%d]: '%s'\n",
				progname, c, c, commandv[c]);
	}

	first_interval = validate_duration(commandv[0]);

	/* 
	 * Shift the commandv[] array in one element, so we will have
	 * a pure string to pass into execvp().
	 */
	fprintf(doutput, "%s: Shifting commandv[].\n", progname);
	for (c = 1; c < argc; c++) {
		fprintf(doutput, "%s: Shifted commandv[] %d to %d:\n%d: %s\n%d: %s\n",
				progname, c, (c-1), (c-1), commandv[(c - 1)], c, commandv[c]);
		commandv[(c - 1)] = commandv[c];
	}
	/* 
	 * And then "close" the last two elements on the string.
	 * It would be better if we could shrink the allocation
	 * by removing two elements.
	*/
	commandv[(c-1)]='\0';
	commandv[c]='\0';

	fprintf(doutput, "%s: First interval: (sec: %ld, nsec: %ld)\n%s: Second interval: (sec: %ld, nsec: %ld)\n",
			progname, first_interval.sec, first_interval.nsec,
			progname, second_interval.sec, second_interval.nsec);

	if (! fForeground) {
		/* 
		 * setpgid(pid, pgid)
		 * If both the 'pid' and 'pgid' are equal to zero, setpgid(2)
		 * assumes that both the child process pid and pgid shall be the
		 * same as parent's, that is why it is used in this case, where
		 * we do not want to foreground the process.
		 */
		if ((pgid = setpgid(0, 0)) != 0) {
			/* debug.txt */
			pfmt(doutput, MM_ERROR,
			"%s: failed to set process group via setpgid(0, 0): %s.\n",
			progname, strerror(errno));
			/* /dev/stderr */
			pfmt(stderr, MM_ERROR,
			"%s: failed to set process group via setpgid(0, 0): %s.\n",
			progname, strerror(errno));
		}
	}

	/* 
	 * The list of signals masked (blocked) during the execution of the
	 * handler (which is present at sigaction.sa_handler/sa_sigaction)
	 * must be initialized per excluding the current list at sa_mask.
	 */
	sigemptyset(&sa.sa_mask);
	
	/* 
	 * Since these two signals can not be caught nor ignored (using
	 * sa_mask), its inclusion at the signal ignore array must avoided.
	 * It will not cause any error, though, just be ignored.
	 */
	if (killer_sig != SIGKILL && killer_sig != SIGSTOP) {
		sigsused[0] = killer_sig;
	}

	/*
	 * Iterate over the sigsused[] integer array and start adding
	 * each one to the sa_mask set again.
	 */
	for (s = 0; s < (int)(sizeof(sigsused)/sizeof(sigsused[0])); s++) {
		sigaddset(&sa.sa_mask, sigsused[s]);
	}

	sa.sa_handler = &handle_signal;

	/*
	 * Set SA_RESTART at sa_flags, this will make certain system calls
	 * restartable by signals. This is useful because, after handle_signal()
	 * returns, the system call is restarted instead of failing with EINTR,
	 * which means an error by interruption.
	 */
	sa.sa_flags = SA_RESTART;

	/*
	 * Iterate over the sigsused[] integer array again, but this time it
	 * will be using sigaction() for changing an action taken by the system
	 * when any of the signals of the array --- which also are present on the
	 * handle_signal() function and, as the name explicitaly say, handled by
	 * it --- are sent, so, when any of these signals are sent, it actually
	 * calls handle_signal().
	 */
	for (s = 0; s < (int)(sizeof(sigsused)/sizeof(sigsused[0])); s++) {
		if (sigsused[s] != -1 && sigsused[s] != 0
		&& sigaction(sigsused[s], &sa, NULL) != 0) {
			/* debug.txt */
			pfmt(doutput, MM_ERROR,
				"%s: failed to change action for %s: %s.\n",
            			progname, strsignal(sigsused[s]),
					strerror(errno));
			/* /dev/stderr */
			pfmt(stderr, MM_ERROR,
				"%s: failed to change action for %s: %s.\n",
            			progname, strsignal(sigsused[s]),
					strerror(errno));
			exit(1);
		}
	}

	/* 
	 * This is meant to ignore stop signals that could come from the
	 * terminal on the parent process.
	 */
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);

	exec_pid = fork();

	fprintf(doutput, "%s: fork()'d to %d\n", progname, (int)exec_pid);
	/* Get the bad news first. */
	if (exec_pid == -1) {
		/* debug.txt */
		pfmt(doutput, MM_ERROR, "%s: failed to fork: %s.\n",
            		progname, strerror(errno));
		/* /dev/stderr */
		pfmt(stderr, MM_ERROR, "%s: failed to fork: %s.\n",
        	    progname, strerror(errno));
	} else if (exec_pid == 0) {
		/* 
		 * On the child process, signals sent by the terminal mustn't
		 * be ignored anymore and that the default action --- without
		 * involving the handler --- have to be taken by the system.
		 */
		signal(SIGTTIN, SIG_DFL);
		signal(SIGTTOU, SIG_DFL);

		/* Here we will be executing the child process. */
		if ( (execvp(commandv[0], commandv)) == -1 ) {
			/* debug.txt */
			pfmt(doutput, MM_ERROR, "%s: failed to exec(): %s.\n",
					progname, strerror(errno));
			/* /dev/stderr */
			pfmt(stderr, MM_ERROR, "%s: failed to exec(): %s.\n",
					progname, strerror(errno));

		}

		/*
		 * According to GNU's timeout(1) manual page:
		 *
		 *  126    if COMMAND is found but cannot be invoked
		 *  127    if COMMAND cannot be found
		 * 
		 *  ENOENT stands for "error no entity/entry", so it
		 *  means, in this context, no such file or directory.
		 */
		execerr = (errno == ENOENT) ? 127 : 126;
		_exit(execerr);
	}
	
	/* The command string will not be necessary after exec(). */
	free(commandv);

	/* 
	 * Add signals from the sa_mask set to the SIG_BLOCK set,
	 * blocking it after the execution of the commmand.
	 */
	if (sigprocmask(SIG_BLOCK, &sa.sa_mask, NULL) != 0) {
		/* debug.txt */
		pfmt(doutput, MM_ERROR, "%s: failed to sigprocmask(): %s.\n",
				progname, strerror(errno));
		/* /dev/stderr */
		pfmt(stderr, MM_ERROR, "%s: failed to sigprocmask(): %s.\n",
				progname, strerror(errno));
		exit(1);
	}

	settimeout(&first_interval);

	for (;;) {
		/* 
		 * The sa_mask will be empty again, but we will
		 * also be using sigsuspend() to prevent the
		 * function assigned to sa_handler of
		 * interrupting this section. 
		 */
		sigemptyset(&sa.sa_mask);
		sigsuspend(&sa.sa_mask);

		if (siglist.sig_chld) {
			siglist.sig_chld = 0;
			cmdpid = wait(&ecmd);
			for (; (cmdpid < 0 && errno == EINTR); ) {
				continue;
			}

			if (cmdpid == exec_pid) {
				eprog = ecmd;
				break;
			}
		} else if (siglist.sig_alrm || siglist.sig_term) {
			if (siglist.sig_alrm) {
				siglist.sig_alrm = 0;
				timesout = 1;
			}

			if (! fForeground) {
				killpg(pgid, killer_sig);
			} else {
				kill(exec_pid, (siglist.sig_term
						? (int)siglist.sig_term
						: killer_sig));
			}

			if (! fOnemoretime) {
				break;
			} else {
				settimeout(&second_interval);
				fOnemoretime = 0;
				siglist.sig_ign = killer_sig;
				killer_sig = SIGKILL;
			}

		}

	}

	for (; (cmdpid != exec_pid && wait(&eprog) == -1); ) {
		if (errno != EINTR) {
			/* debug.txt */
			pfmt(doutput, MM_ERROR, "%s: failed to wait(): %s.\n",
			progname, strerror(errno));
			/* /dev/stderr */
			pfmt(stderr, MM_ERROR, "%s: failed to wait(): %s.\n",
			progname, strerror(errno));

			exit(1);
		}
	}
	
	if (WEXITSTATUS(eprog)) {
		eprog = WEXITSTATUS(eprog);
	} else if (WIFSIGNALED(eprog)) {
		eprog = 128 + WTERMSIG(eprog);
	}
	
	if (timesout && !fPreserve_status) {
		eprog = 124;
	}

	/* Close debug file. */
	fclose(doutput); 
	
	return eprog;
}


void settimeout(struct TClock *duration) {
	timer_t tid;
	struct itimerspec its;

	/* Since the second value, sevp on the prototype of
	 * timer_create(2), is defined as NULL, every
	 * expiration of this timer will sent an SIGALRM.
	 * Just as expected.
	 */
	if (timer_create(CLOCK_REALTIME, NULL, &tid) != 0) {
		pfmt(stderr, MM_ERROR,
			"%s: failed to create timer: %s\n",
			progname, strerror(errno));
		exit(1);
	}

	/* Timer expiration. */	
	its.it_value.tv_sec = duration->sec;
	its.it_value.tv_nsec = duration->nsec;
	/* Timer period. */
	its.it_interval.tv_sec = its.it_value.tv_sec;
	its.it_interval.tv_nsec = its.it_value.tv_nsec;

	if (timer_settime(tid, 0, &its, NULL) != 0) {
		pfmt(stderr, MM_ERROR,
			"%s: failed to set timer: %s\n",
			progname, strerror(errno));
		exit(1);
	}
}

int validate_signal(char *str) {
	int signum = 0; /* EXIT if not defined. */

	/* 
	 * Check if the first character
	 * of the input string is a
	 * letter, so it can be parsed into
	 * a signal name.
	 */
	if (isalpha(str[0])) {
		register int s = 0;
		
		/* Starts with "SIG". */
		char *ssuffix = "SIG";
		if (strncmp(str, ssuffix, 3) == 0) {
			/* 
			 * Re-define string 'str'
			 * from its first three
			 * initial characters ("SIG").
			 */
			str = &str[3];
		}

		for (; (strcmp(str, sig_strs[s].signame) != 0);) {
			s += 1;
		}
		signum = sig_strs[s].signum;
	} else {
		int i = 0;
		i = atoi(str);	
		if (i < 1 || i > SIGRTMAX) {
			pfmt(stderr, MM_ERROR,
				"%s: invalid signal %d.\n",
				progname, i);
			exit(1);
		}
		signum = i;
	}

	return signum;
}

struct TClock validate_duration(char *timestr) {
		float time;

		/* Allocate a buffer for the time string, so we can separate it
		 * easier betwixt seconds and nanoseconds. */
		char timebuf[128];
		char *aftersep = NULL;

		/* Struct that will be returned later containing seconds and
		 * microseconds. */
		struct TClock duration;

		time = conv_duration(timestr);
		snprintf(timebuf, sizeof(timebuf), "%.*e",
				(DECIMAL_DIG - 1), time);	

		size_t decsep = 0;
		for (; timebuf[decsep]; decsep++) {
			/* 
			 * '.' is being used as a franc decimal separator, since
			 * it is expected by strtof() and it is also the decimal
			 * separator on English-speaking countries, but this
			 * program still being localized --- see conv_duration()
			 * for more details.
			 */
			if (timebuf[decsep] == '.') {
				timebuf[decsep] = '\0';
				aftersep = &timebuf[decsep + 1];
				break;
			}
		}

		if (strlen(timebuf) == 0) {
			duration.sec = 0;
		} else {
		       	duration.sec = atoi(timebuf);
		}

		size_t afterseplen = aftersep ? strlen(aftersep) : 0;
		if (afterseplen > 0) {
			long int integer = atoi(aftersep);

			if (afterseplen > 9) {
				aftersep[9] = '\0';
				afterseplen = 9;
			}

			for (size_t i = 0; i < 9 - afterseplen; i++) {
				integer *= 10;

				duration.nsec = integer;
			}

			if (duration.sec == 0 && duration.nsec < 100000000) {
				duration.nsec = 100000000;
			}
		}
		
		return duration;
}

float conv_duration(char *timestr) {
	float time;
	char *timeunit;
	
	/* Support both commas and points as decimal separators */
	char *decsep;
	decsep = strchr(timestr, ',');
	if (decsep) {
		*decsep = '.';
	}

	if ( (time = strtof(timestr, &timeunit)) == 0
			&& timestr == timeunit ) {
		pfmt(stderr, MM_ERROR, "%s: time is not a number.\n", progname);
		exit(1);
	} else if ( time < 0 || time >= FLT_MAX ) {
		pfmt(stderr, MM_ERROR, "%s: time exceeds FLT_MAX (%d).\n", progname, FLT_MAX);
		exit(1);
	}

	switch (*timeunit) {
		case '\0':
		case 's':	
			break;
		case 'm':
			time = time * 60;
			break;
		case 'h':
			time = time * (60 * 60);
			break;
		case 'd':
			time = time * ((60 * 60) * 24);
			break;
		default:
			pfmt(stderr, MM_ERROR, "%s: invalid time unit suffix.\n", progname);
			exit(1);
	}

	return time;
}

// This is just a function that sets variables --- which
// indicates which signals where enabled or not ---, so it
// does not return anything. I wish these could be returned
// on something like a struct, like it was done for the
// interval, but the UNIX ABI does not give this opportunity.
void handle_signal(int signo) {
	if (siglist.sig_ign != 0 && signo == siglist.sig_ign) {
		// Ignore this signal.
		siglist.sig_ign = 0;
		return;
	}

	switch (signo) {
		case SIGHUP:
		case SIGINT:
		case SIGQUIT:
		case SIGTERM:
			siglist.sig_term = signo;
			break;
		case SIGCHLD:
			siglist.sig_chld = 1;
			break;
		case SIGALRM:
			siglist.sig_alrm = 1;
			break;
	}
}

void usage(void) {
	pfmt(stderr, MM_NOSTD,
		"usage: %s: [-fp] [-s signal] [-k time] time [command [args ...]]\n",
		progname);
	exit(1);
}
