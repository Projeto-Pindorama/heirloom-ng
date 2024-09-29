/*
 * safeify.c - Make strings safe for printing and beat CVE-2024-28085.
 */
/*
 * Copyright (C) 2024: Luiz Antônio Rangel (takusuman)
 *
 * SPDX-Licence-Identifier: Zlib
 *
 * Sections of the CVE-2024-28085 mitigation borrowed from 4.3BSD
 * Net/2's cat(1) implementation for the '-v' option. It differs
 * from Heirloom native/UNIX 7th's cat(1) per not being so dependent
 * on magic numbers and using functions from the "isx()" family.
 * As per the copyright header of Net/2's "bin/cat/cat.c":
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Kevin Fall.
 *
 * SPDX-Licence-Identifier: BSD-4-Clause-UC
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *ssafe(char *s) {
	int i = 0,
	    j = 0;
	char safe[BUFSIZ] = "",
	     *ssafe = "";

	for (i = 0; s[i]; i++) {
		switch (iscntrl(s[i])) {
			default:
				switch (s[i]) {
					case '\n':
					case '\t':
					case '\r':
					case ' ':
						goto normal;
					default:
						safe[j] = '^';
						safe[(j + 1)] = (s[i] == '\177')
								? '?'
								: (s[i] ^ 0100);
						i++;
						j+=2;
						continue;
					}
				break;
			case 0:
			normal:
				safe[j] = s[i];
				break;
		}
		j++;
	}

	if ((ssafe = calloc(BUFSIZ, sizeof(char *))) == NULL) {
		fprintf(stderr, "%s: Couldn't allocate string of length %d\n",
				__func__, BUFSIZ);
		exit(1);
	}
	ssafe = strdup(safe);

	return ssafe;
}
