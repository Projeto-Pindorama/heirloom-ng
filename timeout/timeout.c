#include <errno.h>
#include <float.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pfmt.h>
#include <wait.h>

char *getenv(const char *);

static char *progname;

int main(int argc, char *argv[]);
float validate_duration(char *duration);
//int validate_signal(char *signal_name); 
//void handle_signal(int signal_number);
void usage(void);

/*
 * Rodar o comando e bifurcar para o fundo
 * "Matar" o comando depois de X tempo com SIGTERM
 * Se ainda estiver rodando, matar em definitivo com
 * o sinal informado pelo usuário --- esperar por mais
 * tempo? Opção '-k'.
 */

int main(int argc, char *argv[]) {
	progname = argv[0];

	/* Program options and flags. */
	int option = 0,
	    Fforeground = 0,
	    Fpreserve_status = 0;
	
	int c = 0,
	    err = 0, /* Command error code. */
	    ec = 0, /* Command exit code. */
	    timesout = 0;
	
	float first_interval = 0,
	      second_interval = 0;
	char **commandv,
	     *kill_signal;
	pid_t exec_pid;

	/* Debug block. */
	FILE *doutput;

	if (getenv("HEIRLOOM_DEBUG")) {
		if ((doutput = fopen("debug.txt", "w")) == NULL) {
			pfmt(stderr, MM_ERROR, "failed to open debug.txt.\n");
		}
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
				Fforeground = 1;
				break;
			case 'p':
				/* Simplest option overall, it catches process'
				 * exit code and returns it instead of returning
				 * timeout's default "timed out" exit code (124).
				 * This is only valid for processes that did not
				 * exit as zero.
				 */
				Fpreserve_status = 1;
				break;
			case 's':
				kill_signal = optarg;
				break;
			case 'k':
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
		exit(-1);
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

	fprintf(doutput, "%s: First interval: %f\n%s: Second interval: %f\n",
			progname, first_interval, progname, second_interval);
	
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
		err = (errno == ENOENT) ? 127 : 126;
		_exit(err);
	}

	/* I believe the string will not be necessary after exec(). */
	free(commandv);

	waitpid(exec_pid, &ec, 0); 

	/* Close debug file. */
	fclose(doutput); 
	
	return 0;
}

float validate_duration(char *timestr) {
	float time;
	char *timeunit;

	if ( (time = strtod(timestr, &timeunit)) == 0
			&& timestr == timeunit ) {
		pfmt(stderr, MM_ERROR, "%s: time is not a number.\n", progname);
		exit(1);
	} else if ( time < 0 || time >= FLT_MAX ) {
		pfmt(stderr, MM_ERROR, "%s: time exceeds FLT_MAX (%f).\n", progname, FLT_MAX);
		exit(1);
	}

	switch (*timeunit) {
		case '\0':
		case 's':	
			break;
		case 'm':
			time = (time * 60);
			break;
		case 'h':
			time = (time * (60 * 60));
			break;
		case 'd':
			time = (time * ((60 * 60) * 24));
			break;
		default:
			pfmt(stderr, MM_ERROR, "%s: invalid time unit suffix.\n", progname);
			exit(1);
	}

	return time;
}

void usage(void) {
	pfmt(stderr, MM_NOSTD,
		"usage: %s: [-fp] [-s signal] [-k time] time [command [args ...]]\n",
		progname);
	exit(1);
}
