/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*
 * Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)fgetline.c	1.4 (gritter) 6/25/05
 */

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

void *
srealloc(void *p, size_t size)
{
	if ((p = realloc(p, size)) == NULL) {
		write(2, "mail: Can't malloc\n", 19);
		_exit(0177);
	}
	return p;
}

#define	LSIZE	128	/* initial line size */

#if defined (__GLIBC__) && defined (_IO_getc_unlocked)
#undef	getc
#define	getc(f)	_IO_getc_unlocked(f)
#endif

char *
fgetline(char **line, size_t *linesize, size_t *llen, FILE *fp)
{
	int c;
	size_t n = 0;

	if (*line == NULL || *linesize < LSIZE + n + 1)
		*line = srealloc(*line, *linesize = LSIZE + n + 1);
	for (;;) {
		if (n >= *linesize - LSIZE / 2)
			*line = srealloc(*line, *linesize += LSIZE);
		c = getc(fp);
		if (c != EOF) {
			(*line)[n++] = c;
			(*line)[n] = '\0';
			if (c == '\n')
				break;
		} else {
			if (n > 0)
				break;
			else
				return NULL;
		}
	}
	if (llen)
		*llen = n;
	return *line;
}

#ifdef	__hpux
int
seteuid(uid_t euid)
{
	return euid ? setuid(euid) : -1;
}

int
setegid(gid_t egid)
{
	return setgid(egid);
}
#endif	/* __hpux */
