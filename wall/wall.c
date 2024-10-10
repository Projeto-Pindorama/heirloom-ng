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

#pragma clang diagnostic ignored "-Wmain-return-type"
#include <errno.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strmenta.h>
#include <sys/types.h>
#include <unistd.h>
#include <utmp.h>
#include <utmpx.h>
#define MAXNAMLEN _POSIX_LOGIN_NAME_MAX

char	*mesg = "",
	who[MAXNAMLEN] = "???",
	sterm[32] = "";
int	msize = 0;
void main(int argc, char *argv[]);
void sendmes(char *tty);

void main(int argc, char *argv[]) {
	int i = 0;
	char c = '\0',
	     smesg[3000] = "";
	struct utmpx *utmp;
	struct passwd *pw;
	FILE *f = stdin;

	if((utmp = getutxent()) == NULL) {
		fprintf(stderr, "failed to open utmpx database: %s\n",
			strerror(errno));
		exit(1);
	}

	if(argc >= 2) {
		/* take message from unix file instead of standard input */
		if((f = fopen(argv[1], "r")) == NULL) {
			fprintf(stderr, "Cannot open %s\n", argv[1]);
			exit(1);
		}
	}

	while ((c = getc(f)) != EOF) smesg[msize++] = c;
	fclose(f);
	mesg = ssafe(smesg);

	pw = getpwuid(geteuid());
	if(pw) {
		for (i = 0; (c = pw->pw_name[i]); i++) who[i]=c;
		who[i] = '\0'; /* sender initials */
	}

	/* Get the current terminal */
	strncpy(sterm, ttyname(fileno(stderr)), sizeof(sterm));
	for (i = 1; i < sizeof(sterm) && sterm[i] != '/'; i++);
	strncpy(sterm, &sterm[(i + 1)], UT_LINESIZE);
	sterm[(UT_LINESIZE - 1)] = '\0';

	/* Rewind utmpx file to its start,
	 * like a cassete tape. */
	setutxent();
	for (; utmp != NULL; utmp = getutxent()) {
		switch (utmp->ut_type) {
			case USER_PROCESS:
				sleep(1);
				sendmes(utmp->ut_line);
				break; /* A simple formality. */
			default:
				continue;
		}
	}
	/* Close utmpx file. */
	endutxent();

	exit(0);
}

void sendmes(char *tty) {
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
	strncat(t, tty, UT_LINESIZE);

	if((f = fopen(t, "w")) == NULL) {
		fprintf(stderr, "cannot send to %s: %s\n",
				tty, strerror(errno));
		_exit(1);
	}

	setbuf(f, buf);
	fprintf(f, "\7\7\7Broadcast Message from %s (%s) ...\n\n",
			who, sterm);
	fwrite(mesg, msize, 1, f);
	fclose(f);

	_exit(0);
}
