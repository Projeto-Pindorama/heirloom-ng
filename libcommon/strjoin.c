#include <stdio.h>
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
	printf("%d\n", joinlen);
	if (en == 1) return elems[0];

	/*
	 * Compute the number of times that the separator
	 * will appear amid the strings and sum it to the
	 * length of the new, joint string.
	 * Also have one more for '\0'.
	 */
	joinlen += ((strlen(sep) * (en - 1)) + 1);
	printf("%d\n", joinlen);

//	joinbuf = calloc(joinlen, sizeof(char *));
//	joinbufp = joinbuf;

	for (size_t i = 0; i < en; i++) {
		puts(elems[i]);
		puts(sep);
	}
	return NULL;
}
