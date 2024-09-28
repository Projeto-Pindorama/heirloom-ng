/*
 * write.c - write to another user
 *
 * Copyright (C) 2024: Luiz Antônio Rangel (takusuman)
 *
 * SPDX-Licence-Identifier: Zlib
 *
 *	from Unix 32V /usr/src/cmd/write.c
 *	November 6th, 1978.
 *
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * SPDX-Licence-Identifier: Caldera
 */

#include <fcntl.h>
#include <pwd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <utmpx.h>

int	signum[] = {SIGHUP, SIGINT, SIGQUIT, 0};
char	me[10]	= "???",
	*mytty = "",
	*him = "",
	histty[32] = "",
	*histtya = "";
FILE	*tf;

void	main(int argc, char *argv[]);
void	eof(void);
void	timout(void);
int	ex(char* bp);
int	sigs(void (*sig)(int));
void	usage(void);

void main(int argc, char *argv[]) {
	int i = 0,
	    c = 0,
	    tfd = 0,
	    logcnt = 0;
	struct passwd *passw;
	struct utmpx *u;
	struct stat stbuf;

	if (argc < 2)
		usage();
	him = argv[1];
	if (argc > 2)
		histtya = argv[2];
	passw = getpwuid(geteuid());
	if (passw) {
		for (i = 0; c = passw->pw_name[i]; i++) me[i]=c;
		me[(i + 1)] = '\0'; /* sender initials */
	} else {
		fprintf(stderr, "Can't open password database file\n");
		exit(1);
	}
	mytty = ttyname(fileno(stderr));
	if (mytty == NULL) {
		fprintf(stderr, "Can't find your tty\n");
		exit(1);
	}

	/* Get everything after /dev. */
	for (i = 1; mytty[i] != '/'; i++);
	mytty += (i + 1);
	if (histtya[0]!='\0') {
		strcpy(histty, "/dev/");
		strcat(histty, histtya);
	}

	setutxent();
	while ((u = getutxent()) != NULL) {
		if(him[0] != '-' || him[1] != 0)
		switch (strncmp(him, u->ut_user, 9)) {
			case 0:
				break;
			default:
				continue; /* nomat */

		}
		logcnt++;
		if (histty[0] == '\0') {
			strcpy(histty, "/dev/");
			strcat(histty, u->ut_line);
		}
	}
	endutxent();
cont:
	if (logcnt==0 && histty[0]=='\0') {
		fprintf(stderr, "%s not logged in.\n", him);
		exit(1);
	}
	if (histtya==0 && logcnt > 1) {
		fprintf(stderr, "%s logged more than once\nwriting to %s\n",
					him, histty);
	}
	if(histty[0] == '\0') {
		fprintf(stderr, him);
		if(logcnt)
			fprintf(stderr, " not on that tty\n");
		else
			fprintf(stderr, " not logged in\n");
		exit(1);
	}
	signal(SIGALRM, timout);
	alarm(5);
	if ((tf = fopen(histty, "w")) == NULL)
		goto perm;
	alarm(0);
	tfd = fileno(tf);
	if (isatty(tfd) < 0) {
		fprintf(stderr, "No such tty\n");
		exit(1);
	}
	if (fstat(tfd, &stbuf) < 0)
		goto perm;
	/* 
	 * Check if the sender user's group (as per S_IWGRP)
	 * or everyone (as per S_IWOTH) can write to
	 * receiver's terminal.
	 */
	if (!(stbuf.st_mode & (S_IWGRP|S_IWOTH)))
		goto perm;
	sigs(eof);
	fprintf(tf, "Message from ");
#ifdef interdata
	fprintf(tf, "(Interdata) " );
#endif
	fprintf(tf, "%s %s...\n", me, mytty);
	fflush(tf);
	for(;;) {
		char buf[128] = "";
		i = read(0, buf, 128);
		if(i <= 0)
			eof();
		if(buf[0] == '!') {
			buf[i] = 0;
			ex(buf);
			continue;
		}
		write(tfd, buf, i);
	}

perm:
	fprintf(stderr, "Permission denied\n");
	exit(1);
}

void timout(void) {
	fprintf(stderr, "Timeout opening his tty\n");
	exit(1);
}

void eof(void) {
	fprintf(tf, "EOF\n");
	exit(0);
}

int ex(char *bp) {
	int i = 0;

	sigs(SIG_IGN);
	i = fork();
	if(i < 0) {
		fprintf(stderr, "Try again\n");
		goto out;
	}
	if(i == 0) {
		sigs((int (*)())0);
		execl(SHELL, "sh", "-c", bp+1, 0);
		exit(0);
	}
	while(wait((int *)NULL) != i)
		;
	printf("!\n");
out:
	sigs(eof);
}

int sigs(void (*sig)(int)) {
	int i = 0;

	for(i=0; signum[i]; i++)
		signal(signum[i], sig);
}

void usage(void) {
	fprintf(stderr, "usage: write user [ttyname]\n");
	exit(1);
}
