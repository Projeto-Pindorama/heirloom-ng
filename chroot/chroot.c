/* 
 * chroot.c - change the root directory for a command
 */
/*
* Copyright (C) 2022: Luiz Antônio (takusuman)
* 		      Arthur Bacci (arthurbacci)
*             Molly A. McCollum (hex7c)
*
* SPDX-Licence-Identifier: Zlib
*/

#include <errno.h>
#include <pfmt.h>
#include <prerror.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define ROOTUID 0

int main(int argc, char *argv[]) {
	
	if (argc < 2) {
		fprintf(stderr, "usage: %s <newroot> [command]\n", argv[0]);
		exit(1);
	}

	// chroot(8) will only run if the user is root, according to the
	// specification.
	if (geteuid() != ROOTUID) {
		pfmt(stderr, MM_ERROR, "not running as superuser.\n");
		exit(2);
	}

	if (argc < 3) {
		if (
			chroot(argv[1]) < 0 ||
			chdir("/") < 0 ||
			execv("/bin/sh", "-i") < 0
		) {
			prerror(errno);
			exit(3);
		}

	} else if (
		chroot(argv[1]) < 0 ||
		chdir("/") < 0 ||
		execv(argv[2], &argv[2]) < 0
	) {
		prerror(errno);
		exit(3);
	}
}
