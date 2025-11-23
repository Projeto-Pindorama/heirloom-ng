/*
 * strjoin.c - concatenates strings from an array into a single one
 *
 * Mildly inspired by Go standard library's strings.Join function.
 *
 * Copyright (c) 2025 Luiz Antônio Rangel (takusuman)
 *
 * SPDX-Licence-Identifier: Zlib
 */

#include <stdlib.h>
#include <string.h>

char *strjoin(const char *elems[], char *sep) {
 	size_t en = 0,
	       i = 0,
	       joinlen = 0;
	const char **elem = NULL;
	char *joinbuf = NULL,
	     *joinbufp = NULL;

	for (elem = elems; *elem; elem++, en++)
		joinlen += strlen(*elem);
	joinlen += ((strlen(sep) * (en - 1)) + 1);

	/* Populate the buffer. */
	joinbuf = calloc(joinlen, sizeof(char));
	joinbufp = joinbuf;

	for (i = 0; i < en; i++) {
		strcat(joinbufp, elems[i]);
		if (i < (en - 1))
			strcat(joinbufp, sep);
	}

	return joinbuf;
}
