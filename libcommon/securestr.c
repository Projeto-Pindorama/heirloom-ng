/*
 * securestr.c - Make strings safe for printing and beat CVE-2024-28085.
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
#include <string.h>
#include <stdbool.h>

char *ssafe(const char *s) {
	const char *const whitespace_chars = "\n\t\r ";

	char safe[BUFSIZ];
	size_t i = 0;

	for (; *s; s++) {
		bool is_ctrl = iscntrl(*s);
		bool is_whitespace = NULL != strchr(whitespace_chars, *s);

		if (!is_ctrl || is_whitespace)
			safe[i++] = *s;
		else {
			safe[i++] = '^';
			// This ternary converts escape characters to letters
			// by adding 64 to it. For example, LF (line feed) is
			// 10 in decimal. Plus 64 it is 74, that corresponds to
			// the uppercase letter J. Pressing Ctrl-J on your
			// terminal usually inserts a newline indeed.
			// However, an exclusive or is being used instead of
			// addition, that's because of DEL (127 in decimal). If
			// we were to add 64 to it it would overflow to a
			// negative character (not part of ascii), and that's
			// not what is wanted. By using an exclusive or DEL is
			// correctly encoded into a interrogation mark (^?)
			// without affecting the other escapes.
			safe[i++] = *s ^ 0100; // 64 in octal
		}
	}
	safe[i] = '\0';

	return strdup(safe);
}
