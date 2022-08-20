/* 
 * chroot.c - change the root directory for a command
 */
/*
* Copyright (C) 2022: Luiz Antônio (takusuman)
* 		      Arthur Bacci (arthurbacci)
*
* SPDX-Licence-Identifier: Zlib
*/

#include <errno.h>
#include <pfmt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define ROOTUID 0

void print_error(int error) {
	errno = 0;
	char *str = strerror(error);
	if (errno != 0)
		pfmt(stderr, MM_ERROR, "errno %d\n", error);
	else
		pfmt(stderr, MM_ERROR, "%s\n", str);
}

int main(int argc, char *argv[]) {
	if (argc < 3) {
		fprintf(stderr, "usage: %s <newroot> <command>\n", argv[0]);
		exit(1);
	}

	// chroot(1M) will only run if the user is root, according to the
	// specification.
	if (geteuid() != ROOTUID) {
		pfmt(stderr, MM_ERROR, "not running as superuser.\n");
		exit(2);
	}

	if (
		chroot(argv[1]) < 0 ||
		chdir("/") < 0 ||
		execv(argv[2], &argv[2]) < 0
	) {
		print_error(errno);
		exit(3);
	}	
}
