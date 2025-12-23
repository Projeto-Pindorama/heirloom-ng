/*
 * strjoin.c - concatenates strings from an array into a single one
 *
 * Mildly inspired by Go standard library's strings.Join function.
 *
 * Copyright (c) 2025 Luiz Antônio Rangel (takusuman)
 *
 * Kudos for Arthur Bacci for some notes on (not using)
 * 'strcat' and 'calloc', and for making the function
 * more efficient in general.
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

	/*
	 * Populate the buffer.
	 * There's no actual necessity for nullifying it
	 * since we've already estimated its size to fit
	 * the new string perfectly, ergo an unnecessary
	 * operation is saved.
	 */
	joinbuf = malloc((joinlen * sizeof(char)));
	joinbufp = joinbuf;

	for (i = 0; i < en; i++) {
		joinbufp = stpcpy(joinbufp, elems[i]);
		if (i < (en - 1))
			joinbufp = stpcpy(joinbufp, sep);
	}

	return joinbuf;
}
