#include <stdio.h>
#include <utmp.h>
#define	USERS	50

char	mesg[3000];
int	msize,sline;
struct	utmp utmp[USERS];
char	*strcpy();
char	*strcat();
char who[9] = "???";

main(argc, argv)
char *argv[];
{
	register i;
	register char c;
	register struct utmp *p;
	FILE *f;

	if((f = fopen("/etc/utmp", "r")) == NULL) {
		fprintf(stderr, "Cannot open /etc/utmp\n");
		exit(1);
	}
	fread((char *)utmp, sizeof(struct utmp), USERS, f);
	fclose(f);
	f = stdin;
	if(argc >= 2) {
		/* take message from unix file instead of standard input */
		if((f = fopen(argv[1], "r")) == NULL) {
			fprintf(stderr,"Cannot open %s\n", argv[1]);
			exit(1);
		}
	}
	while((i = getc(f)) != EOF) mesg[msize++] = i;
	fclose(f);
	sline = ttyslot(2); /* 'utmp' slot no. of sender */
	if (sline) {
		for (i=0;c=utmp[sline].ut_name[i];i++)
			who[i]=c;
		who[i] = '\0'; /* sender initials */
		}
	for(i=0; i<USERS; i++) {
		p = &utmp[i];
		if(p->ut_name[0] == 0)
			continue;
		sleep(1);
		sendmes(p->ut_line);
	}
	exit(0);
}

sendmes(tty)
char *tty;
{
	register i;
	char t[50], buf[BUFSIZ];
	FILE *f;

	i = fork();
	if(i == -1) {
		fprintf(stderr, "Try again\n");
		return;
	}
	if(i)
		return;
	strcpy(t, "/dev/");
	strcat(t, tty);

	if((f = fopen(t, "w")) == NULL) {
		fprintf(stderr,"cannot open %s\n", t);
		exit(1);
	}
	setbuf(f, buf);
	fprintf(f, "Broadcast Message from %s (%s) ...\n\n",who,
		utmp[sline].ut_line);
	fwrite(mesg, msize, 1, f);
	exit(0);
}
