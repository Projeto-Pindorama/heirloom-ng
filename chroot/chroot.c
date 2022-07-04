/* chroot - change root directory for a command
 * Copyright (C) 2022: Luiz Antônio (takusuman).
 */
#include <pfmt.h>
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
#define NOPERM "not running as super-user.\n"
#define ERRCHROOT "could not change root to %s.\n"
#define ERREXEC "could not execute %s.\n"

void
main(int argc, char *argv[])
{
	setlabel(argv[0]);
	if (argc < MIN_ARGS){
		fprintf(stderr, USAGE, argv[0]);
		exit(1);
	}
	if (geteuid() != ROOT_UID){
		pfmt(stderr, MM_ERROR, NOPERM);
		exit(2);
	}
	/* Change root to argv[1], then change the directory into / and execute
	 * argv[2].*/
	/* I think we may need a way to in fact print errors instead of just
	 * saying "ERROR: etc etc etc", but without dropping pfmt.h. Well,
	 * that's a TODO! :^) */
	if (chroot(argv[1]) < 0 || chdir("/") < 0){
		pfmt(stderr, MM_ERROR, ERRCHROOT, argv[1]);
		exit(3);
	}	
	if (execv(argv[2], &argv[2]) < 0){
		pfmt(stderr, MM_ERROR, ERREXEC, argv[2]);
		exit(3);
	}
}
