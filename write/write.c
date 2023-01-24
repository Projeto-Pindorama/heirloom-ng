/*
 * write to another user
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <utmp.h>

char	*strcat();
char	*strcpy();
struct	utmp ubuf;
int	signum[] = {SIGHUP, SIGINT, SIGQUIT, 0};
char	me[10]	= "???";
char	*him;
char	*mytty;
char	histty[32];
char	*histtya;
char	*ttyname();
char	*rindex(), *index();
int	logcnt;
int	eof();
int	timout();
FILE	*tf;

main(argc, argv)
char *argv[];
{
	struct stat stbuf;
	register i;
	register FILE *uf;
	int c1, c2;

	if(argc < 2) {
		printf("usage: write user [ttyname]\n");
		exit(1);
	}
	him = argv[1];
	if(argc > 2)
		histtya = argv[2];
	if ((uf = fopen("/etc/utmp", "r")) == NULL) {
		printf("cannot open /etc/utmp\n");
		goto cont;
	}
	mytty = ttyname(2);
	if (mytty == NULL) {
		printf("Can't find your tty\n");
		exit(1);
	}
	mytty = index(mytty+1, '/') + 1;
	if (histtya) {
		strcpy(histty, "/dev/");
		strcat(histty, histtya);
	}
	while (fread((char *)&ubuf, sizeof(ubuf), 1, uf) == 1) {
		if (strcmp(ubuf.ut_line, mytty)==0) {
			for(i=0; i<8; i++) {
				c1 = ubuf.ut_name[i];
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
			c2 = ubuf.ut_name[i];
			if(c1 == 0)
				if(c2 == 0 || c2 == ' ')
					break;
			if(c1 != c2)
				goto nomat;
		}
		logcnt++;
		if (histty[0]==0) {
			strcpy(histty, "/dev/");
			strcat(histty, ubuf.ut_line);
		}
	nomat:
		;
	}
cont:
	if (logcnt==0 && histty[0]=='\0') {
		printf("%s not logged in.\n", him);
		exit(1);
	}
	fclose(uf);
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
		printf("%s: ", histty);
		printf("No such tty\n");
		exit(1);
	}
	signal(SIGALRM, timout);
	alarm(5);
	if ((tf = fopen(histty, "w")) == NULL)
		goto perm;
	alarm(0);
	if (fstat(fileno(tf), &stbuf) < 0)
		goto perm;
	if ((stbuf.st_mode&02) == 0)
		goto perm;
	sigs(eof);
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

timout()
{

	printf("Timeout opening his tty\n");
	exit(1);
}

eof()
{

	fprintf(tf, "EOF\n");
	exit(0);
}

ex(bp)
char *bp;
{
	register i;

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

sigs(sig)
int (*sig)();
{
	register i;

	for(i=0;signum[i];i++)
		signal(signum[i],sig);
}
