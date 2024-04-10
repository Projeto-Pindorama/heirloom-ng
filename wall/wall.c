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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utmpx.h>

char	mesg[3000];
int	msize, sline;
struct	utmpx utmp[];
char	*strcpy();
char	*strcat();
pid_t   fork();
char who[9] = "???";

void main(int argc, char *argv[]) {
	register unsigned int i;
	register char c;
	unsigned int utmpentr;
	FILE *f;
	struct utmpx *p;

	memcpy(&utmp, getutxent(), sizeof(struct utmpx));

	/* message text from standard input per default */
	f = stdin;
	if (argc >= 2) {
		/* take message from unix file instead of standard input */
		if((f = fopen(argv[1], "r")) == NULL) {
			fprintf(stderr, "Cannot open %s\n", argv[1]);
			exit(1);
		}
	}
	while((i = getc(f)) != EOF) {
		mesg[msize++] = i;
	}
	fclose(f);

	sline = ttyslot(); /* 'utmp' slot no. of sender */
	if (sline) {
		for (i=0; c=utmp[sline].ut_user[i]; i++) {
			who[i]=c;
		}
		who[i] = '\0'; /* sender initials */
	}

/*	for(i=0; utmp[i].ut_line; i++) { */
	for(i=0; utmp[i].ut_user; i++) {
		p = &utmp[i];
                if(p->ut_user[0] == 0)
			continue;
		sleep(1);
		sendmes(p->ut_line);
	}

	exit(0);
}

void sendmes(char *tty) {
	register int i;
        int fd;
	char t[50], buf[BUFSIZ];
	FILE *f;

	i = fork();
	if (i == -1) {
		fprintf(stderr, "Try again\n");
		return;
	}
	if (i)
		return;
	strcpy(t, "/dev/");
	strcat(t, tty);

	fd = open(t, O_WRONLY|O_NOCTTY|O_NONBLOCK);
	if (fd == -1) {
		fprintf(stderr,"cannot open %s: %s\n",
				t, strerror(errno));
		exit(1);
	} else {
		if (!isatty(fd)) {
			fprintf(stderr, "cannot send to %s: not a tty\n", t);
			exit(1);
		}
	}
	
	f = fdopen(fd, "w");
	if (f == NULL) {
		fprintf(stderr,"cannot open file descriptor %d: %s\n",
				fd, strerror(errno));
		exit(1);
	}

	setbuf(f, buf);
	fprintf(f, "Broadcast Message from %s (%s) ...\n\n",
			who, utmp[sline].ut_line);
	fwrite(mesg, msize, 1, f);
	close(fd);
	exit(0);
}
