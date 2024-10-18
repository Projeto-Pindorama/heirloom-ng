/*
 * sputs.h - A puts(3)(-oid) set of macros, but fo strings
 * instead of the standard output.
 *
 * Copyright (c) 2024 Luiz Antônio Rangel (takusuman)
 *
 * SPDX-Licence-Identifier: Zlib
 */

#define sputchar(s, c) *(s++) = c
#define sputs(ds, s)	\
	for (unsigned int sp = 0; s[sp] != '\0'; sp++) \
		sputchar(ds, s[sp]);	\
	sputchar(ds, '\n');
