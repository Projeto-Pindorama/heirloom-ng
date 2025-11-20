/*
 * putz.c - A puts(3)(-oid) that has a trailling null instead of a newline.
 *
 * Copyright (c) 2025 Luiz Antônio Rangel (takusuman)
 *
 * SPDX-Licence-Identifier: Zlib
 */

#include <stdio.h>

inline int putsc(char *restrict s, char trailling) {
	int r1, r2 = 0;
	r1 = fputs(s, stdout);
	r2 = putchar(trailling);
	return (r1 && r2);
}

int putz(char *restrict s) {
	return putsc(s, '\0');
}
