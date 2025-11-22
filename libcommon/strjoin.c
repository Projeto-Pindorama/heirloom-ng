/*
 * strjoin.c - concatenates strings from an array into a single one
 *
 * Mildly inspired by Go standard library's strings.Join function,
 * but a little bit more rigorous to use since, well, this is C.
 *
 * Copyright (c) 2025 Luiz Antônio Rangel (takusuman)
 *
 * SPDX-Licence-Identifier: Zlib
 */

#include <stdlib.h>
#include <string.h>

char *strjoin(char *elems[], char *sep) {
 	size_t en = 0,
	       joinlen = 0;
	char *joinbuf = NULL,
	     *joinbufp = NULL;

	/*
	 * Count strings in the array using 'NULL' as a sentinel.
	 * Also already compute the length of each string in
	 * the array if possible so we can save a loop later.
	 */
	for (; elems[en] != NULL; en++)
		joinlen += strlen(elems[en]);
	if (en == 1) return elems[0];

	/*
	 * Compute the number of times that the separator
	 * will appear amid the strings and sum it to the
	 * length of the new, joint string.
	 * Also have one more for '\0'.
	 */
	joinlen += ((strlen(sep) * (en - 1)) + 1);

	/* Populate the buffer. */
	joinbuf = calloc(joinlen, sizeof(char *));
	joinbufp = joinbuf;

	for (size_t i = 0; i < en; i++) {
		strcat(joinbufp, elems[i]);
		if (i < (en - 1)) strcat(joinbufp, sep);
	}

	return joinbuf;
}
