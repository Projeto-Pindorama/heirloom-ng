/*
* chroot - change root directory for a command
* 
* Luiz Antônio (takusuman), Gramado/RS, Brazil, July 2022.
* /

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Create macros containing the UID for the `root' user and the minimal number
 * of arguments to be passed --- which is 3 because argc counts from the binary
 * name to arguments. */ 
#define ROOT_UID 0
#define MIN_ARGS 3 
/* Define some messages. */
#define USAGE "[usage]: %s newroot command\n"
#define NOPERM "%s: not running as super-user.\n"

void
main(int argc, char *argv[])
{
	if (argc < MIN_ARGS){
		printerr(argv[0], USAGE, 1);
	}
	if (geteuid() != ROOT_UID){
		printerr(argv[0], NOPERM, 2);
	}
	/* Change root to argv[1], then change the directory into / and execute
	 * argv[2].*/
	chroot(argv[1]);
	chdir("/");
	execv(argv[2], &argv[2]);
}

void
printerr(char *progname[], char *msg[], int errcode)
{
	printf(msg, progname);
	exit(errcode);
}
