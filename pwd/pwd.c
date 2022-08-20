/*
 * pwd - print working directory
 *
 * A little bit based on Ritter's implementation
 */
/*
 * Copyright (c) 2005 Gunnar Ritter
 * Copyright (C) 2022 Arthur Bacci
 *
 * SPDX-Licence-Identifier: Zlib
 */

#include <errno.h>
#include <pfmt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* The maximum size `buf` can take up, this check prevents the program from
   crashing your computer */
#define MAXIMUM_SIZE 65536

/* TODO: move this function to <pfmt.h> since it is really useful */
void print_error(int error) {
	errno = 0;
	char *str = strerror(error);
	if (errno != 0)
		pfmt(stderr, MM_ERROR, "errno %d\n", error);
	else
		pfmt(stderr, MM_ERROR, "%s\n", str);
}

int main(void) {
	size_t size = 256;
	char *cwd;
	char *buf = malloc(size);

	while ((cwd = getcwd(buf, size)) == NULL && errno == ERANGE) {
		if ((size *= 2) > MAXIMUM_SIZE) {
			errno = ENAMETOOLONG;
			break;
		}
		buf = realloc(buf, size);
		if (buf == NULL) break;
	}

	if (cwd == NULL) {
		print_error(errno);
		return 1;
	}
	/* ELSE */
	puts(cwd);

	/* Not needed to free the allocated buf since the program ends here */
	
	return 0;
}
