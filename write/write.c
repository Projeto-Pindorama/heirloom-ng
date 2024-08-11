/*
 * write.c - write to another user
 *
 * Copyright (C) 2024: Luiz Antônio Rangel (takusuman)
 *
 * SPDX-Licence-Identifier: Zlib
 *
 *	from Unix 32V /usr/src/cmd/write.c	
 *	Novemberr 6th, 1978.
 *
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * SPDX-Licence-Identifier: Caldera
 */

#include <fcntl.h>
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

char	*strcat();
char	*strcpy();
struct	utmpx *ubuf;
int	signum[] = {SIGHUP, SIGINT, SIGQUIT, 0};
char	me[10]	= "???";
char	*him;
char	*mytty;
char	histty[32];
char	*histtya;
char	*ttyname();
char	*rindex();
int	logcnt;
FILE	*tf;

void	main(int argc, char *argv[]);
int	eof(void);
int	timout(void);
int	ex(char* bp);
void	sigs(int (*sig)());
void	usage(void);

void main(int argc, char *argv[]) {
	struct stat stbuf;
	register int i;
	int c1, c2, tfd;

	if (argc < 2) {
		usage();
	}
	him = argv[1];
	if (argc > 2) {
		histtya = argv[2];
	}
	
	mytty = ttyname(2);
	if (mytty == NULL) {
		printf("Can't find your tty\n");
		exit(1);
	}
	mytty = rindex(mytty, '/') + 1;
	if (histtya) {
		strcpy(histty, "/dev/");
		strcat(histty, histtya);
	}
	while ((ubuf = getutxent()) != NULL) { 
		if (strcmp(ubuf[0].ut_line, mytty)==0) {
			for(i=0; i<8; i++) {
				c1 = ubuf->ut_user[i];
				if(c1 == ' ')
					c1 = 0;
				me[i] = c1;
				if(c1 == 0)
					break;
			}
		}
		if(him[0] != '-' || him[1] != 0)
		for(i=0; i<8; i++) {
			c1 = him[i];
			c2 = ubuf->ut_user[i];
			if(c1 == 0)
				if(c2 == 0 || c2 == ' ')
					break;
			if(c1 != c2)
				goto nomat;
		}
		logcnt++;
		if (histty[0]==0) {
			strcpy(histty, "/dev/");
			strcat(histty, ubuf->ut_line);
		}
	nomat:
		;
	}
cont:
	if (logcnt==0 && histty[0]=='\0') {
		printf("%s not logged in.\n", him);
		exit(1);
	}
	if (histtya==0 && logcnt > 1) {
		printf("%s logged more than once\nwriting to %s\n", him, histty+5);
	}
	if(histty[0] == 0) {
		printf(him);
		if(logcnt)
			printf(" not on that tty\n"); else
			printf(" not logged in\n");
		exit(1);
	}
	if (access(histty, 0) < 0) {
		printf("No such tty\n");
		exit(1);
	}
	signal(SIGALRM, timout);
	alarm(5);
/*	if ((tf = fopen(histty, "w")) == NULL) */
	if ((tfd = open(histty, O_WRONLY)) == -1)
		goto perm;
	
	alarm(0);
/*	if (fstat(fileno(tf), &stbuf) < 0) */
	if (fstat(tfd, &stbuf) < 0)
		goto perm;
	if ((stbuf.st_mode&02) == 0)
		goto perm;
	sigs(eof);
	if ((tf = fdopen(dup2(tfd, STDOUT_FILENO), "w")) == NULL)
		goto perm;
	fprintf(tf, "Message from ");
#ifdef interdata
	fprintf(tf, "(Interdata) " );
#endif
	fprintf(tf, "%s %s...\n", me, mytty);
	fflush(tf);
	for(;;) {
		char buf[128];
		i = read(0, buf, 128);
		if(i <= 0)
			eof();
		if(buf[0] == '!') {
			buf[i] = 0;
			ex(buf);
			continue;
		}
		write(fileno(tf), buf, i);
	}

perm:
	printf("Permission denied\n");
	exit(1);
}

int timout(void) {
	printf("Timeout opening his tty\n");
	exit(1);
}

int eof(void) {
	fprintf(tf, "EOF\n");
	exit(0);
}

int ex(char* bp) {
	register int i;

	sigs(SIG_IGN);
	i = fork();
	if(i < 0) {
		printf("Try again\n");
		goto out;
	}
	if(i == 0) {
		sigs((int (*)())0);
		execl("/bin/sh", "sh", "-c", bp+1, 0);
		exit(0);
	}
	while(wait((int *)NULL) != i)
		;
	printf("!\n");
out:
	sigs(eof);
}

void sigs(int (*sig)()) {
	register int i;

	for(i=0; signum[i]; i++)
		signal(signum[i], sig);
}

void usage(void) {
	printf("usage: write user [ttyname]\n");
	exit(1);
}
