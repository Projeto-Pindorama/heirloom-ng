/*
 * basename.c - a basename(3) implementation in case of
 * the target system not being POSIX.1-2001 compliant.
 *
 * Copyright (c) 2025 Luiz Antônio Rangel (takusuman)
 *
 * SPDX-Licence-Identifier: Zlib
 */

/* got UNIX? */
#if defined(__unix__)
#include <unistd.h>
#else
#define _POSIX_VERSION 0
#endif

#if _POSIX_VERSION < 200112L
#include <string.h>
/*
 * A basename() function first appeared in
 * POSIX.1-2001, before that it had to be
 * made manually.
 */
char *basename(const char s[]) {
	register char *name = NULL,
		 *token = NULL;
	char *ss = NULL;

	/*
	 * Always strdup() 's', since executing
	 * strtok on it can give a memory fault
	 * since we would modify a variable that
	 * we're not supposed to.
	 */
#ifdef __unix__
	char *dirsep = "/";
	ss = strdup(s);
#elif defined(_WIN16) || defined(_WIN32) \
	|| defined(__DOS__) || defined(__OS2__)
	char *dirsep = "\\";
	char *drvltrptr = NULL;

	/*
	 * Remove "<Drive letter>:" before the path.
	 */
	if (drvltrptr = strchr(s, ':'))
		ss = strdup(drvltrptr + 1);
	else
		ss = strdup(s);
#endif
	for (token = strtok(ss, dirsep), name = token;
		token = strtok(NULL, dirsep);) {
		if (token == NULL) {
			break;
		} else {
			name = token;
			continue;
		}
	}

	return name;
}
#else
#include <libgen.h>
#endif
