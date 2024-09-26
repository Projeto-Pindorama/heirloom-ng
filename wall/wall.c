/*
 * wall.c - write to all users
 *
 * Copyright (C) 2024: Luiz Antônio Rangel (takusuman)
 *
 * SPDX-Licence-Identifier: Zlib
 *
 *	from Unix 32V /usr/src/cmd/wall.c
 *	December 19th, 1978.
 *
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * SPDX-Licence-Identifier: Caldera
 */

#include <errno.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <utmpx.h>

/*
 * Compatibility for (struct utmpx *)ut_line
 * and UNIX username length.
 * Note: UT_LINESIZE as 12 is from Solaris.
 */
#ifndef UT_LINESIZE
#ifndef __UT_LINESIZE
#define __UT_LINESIZE 12
#endif
#define UT_LINESIZE __UT_LINESIZE
#endif
#define MAXNAMLEN _POSIX_LOGIN_NAME_MAX

char	mesg[3000] = "",
	who[MAXNAMLEN] = "???",
	sterm[PATH_MAX] = "";
int	msize = 0;
char	*strcpy();
char	*strcat();
pid_t	fork();
void main(int argc, char *argv[]);
void sendmes(struct utmpx *u);

void main(int argc, char *argv[]) {
	int i = 0;
	char c = '\0';
	struct utmpx *utmp;
	struct passwd *pw;
	FILE *f = NULL;

	if ((utmp = getutxent()) == NULL) {
		fprintf(stderr, "failed to open utmpx database: %s\n",
			strerror(errno));
		exit(1);
	}

	f = stdin;
	if(argc >= 2) {
		/* take message from unix file instead of standard input */
		if((f = fopen(argv[1], "r")) == NULL) {
			fprintf(stderr, "Cannot open %s\n", argv[1]);
			exit(1);
		}
	}

	/* Cache the text in the mesg[] array */
	for (; ((i = getc(f)) != EOF); ) mesg[msize++] = i;
	fclose(f);

	/* Get sender's username */
	pw = getpwuid(geteuid());
	if (pw) {
		for (i = 0; c = pw->pw_name[i]; i++) who[i]=c;
		who[i] = '\0'; /* sender initials */
	}

	/* Get the current terminal */
	strncpy(sterm, ttyname(fileno(stderr)), sizeof(sterm));

	/* Rewind utmpx file to its start,
	 * like a cassete tape. */
	setutxent();
	for (; utmp = getutxent(); ) {
		switch (utmp->ut_type) {
			case USER_PROCESS:
				if (utmp != NULL) {
					sleep(1);
					sendmes(utmp);
				} else {
					break;
				}
			default:
				continue;
		}
		break;
	}
	/* Close utmpx file. */
	endutxent();

	exit(0);
}

void sendmes(struct utmpx *u) {
	pid_t i = 0;
	char t[50] = "",
		buf[BUFSIZ] = "";
	FILE *f = NULL;

	i = fork();
	if(i == -1) {
		fprintf(stderr, "Try again\n");
		return;
	}
	if(i) return;
	strncpy(t, "/dev/", 5);
	strncat(t, u->ut_line, UT_LINESIZE);

	if((f = fopen(t, "w")) == NULL) {
		fprintf(stderr, "cannot send to %s on %s: %s\n",
				u->ut_user, t, strerror(errno));
		_exit(1);
	}

	setbuf(f, buf);
	fprintf(f, "\7\7\7Broadcast Message from %s (%s) ...\n\n",
			who, sterm);
	fwrite(mesg, msize, 1, f);
	fclose(f);

	_exit(0);
}
