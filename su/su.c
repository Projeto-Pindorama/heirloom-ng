/*
 * su - become-super user or another user
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, May 2001.
 */
/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute
 * it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 */

#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char sccsid[] USED = "@(#)su.sl	1.25 (gritter) 2/21/06";

#include	"config.h"
#include	<sys/types.h>
#include	<sys/wait.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<stdio.h>
#include	<string.h>
#include	<strings.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<libgen.h>
#include	<time.h>
#include	<signal.h>
#include	<pwd.h>
#include	<grp.h>
#include	<termios.h>
#include	<limits.h>
#include	<syslog.h>
#ifdef	__APPLE__
#include	<pam/pam_appl.h>
#elif	PAM
#include	<security/pam_appl.h>
#else	/* !PAM */
#ifdef SHADOW_PWD
#include	<shadow.h>
#endif	/* SHADOW_PWD */
#define	PAM_MAX_RESP_SIZE	512
#endif	/* !PAM */

enum logtype {
	LT_NONE,
	LT_FAIL,
	LT_ALL
};

extern char	**environ;

static int	dash;			/* create login environment */
static int	dofork;			/* fork() before executing shell */
static enum logtype	dosyslog = LT_NONE;	/* log to syslog */
static int	sleeptime = 4;		/* sleep time if failed */
static char	*user;			/* desired login name */
static char	*olduser;		/* old login name */
static char	*progname;		/* argv[0] to main() */
static char	*path;			/* new $PATH */
static char	*dir;			/* new home directory */
static char	*shell;			/* new shell */
static char	*sulog;			/* logging file */
static char	*console;		/* console to log to (optional) */
static struct passwd	*pwd;			/* passwd entry for new user */
static void	(*oldint)(int);		/* old SIGINT handler */
static void	(*oldquit)(int);	/* old SIGQUIT handler */

static const char	defaultfile[] = SUDFL;
static const char	PATH[] = "PATH=/usr/local/bin:/bin:/usr/bin:";
static const char	SUPATH[] =
	"PATH=/usr/local/sbin:/usr/local/bin:/sbin:/usr/sbin:/bin:/usr/bin";

/*
 * Memory allocation with check.
 */
static void *
smalloc(size_t nbytes)
{
	void *p;

	if ((p = malloc(nbytes)) == NULL) {
		write(2, "Out of memory\n", 14);
		exit(077);
	}
	return p;
}

/*
 * Adjust environment.
 */
static void
doenv(void)
{

	if (dash) {
		char *cp;
		char *term, *oldterm;
		char *display, *olddisplay;

		if ((oldterm = getenv("TERM")) != NULL) {
			term = smalloc(strlen(oldterm) + 6);
			sprintf(term, "TERM=%s", oldterm);
		} else
			term = NULL;
		if ((olddisplay = getenv("DISPLAY")) != NULL) {
			display = smalloc(strlen(olddisplay) + 9);
			sprintf(display, "DISPLAY=%s", olddisplay);
		} else
			display = NULL;
		environ = NULL;
		cp = smalloc(strlen(dir) + 6);
		sprintf(cp, "HOME=%s", dir);
		putenv(cp);
		cp = smalloc(strlen(user) + 9);
		sprintf(cp, "LOGNAME=%s", user);
		putenv(cp);
		cp = smalloc(strlen(shell) + 7);
		sprintf(cp, "SHELL=%s", shell);
		putenv(cp);
		if (term)
			putenv(term);
		if (display)
			putenv(display);
	}
	putenv(path);
	if (pwd->pw_uid == 0)
		putenv("PS1=# ");
}

/*
 * Things to do in child process.
 */
static void
child(char **args)
{
	if (initgroups(user, pwd->pw_gid) < 0 || setgid(pwd->pw_gid) < 0
			|| setuid(pwd->pw_uid) < 0) {
		fprintf(stderr, "%s: Invalid ID\n", progname);
		dofork ? _exit(1) : exit(1);
	}
	if (dash) {
		args[0] = smalloc(strlen(basename(shell)) + 2);
		args[0][0] = '-';
		strcpy(&args[0][1], basename(shell));
		if (dir == NULL || chdir(dir) < 0) {
			fprintf(stderr, "%s: No directory! Using home=/\n",
					progname);
			dir = "/";
			chdir(dir);
		}
	} else
		args[0] = basename(shell);
	doenv();
	signal(SIGINT, oldint);
	signal(SIGQUIT, oldquit);
	execv(shell, args);
	fprintf(stderr, "%s: No shell\n", progname);
	dofork ? _exit(3) : exit(3);
}

/*
 * Spawn the child process and wait for it.
 */
static int
dospawn(char **args)
{
	int status;
	pid_t pid;

	switch (pid = fork()) {
	case 0:
		child(args);
		/*NOTREACHED*/
	case -1:
		fprintf(stderr, "%s: fork() failed, try again later\n",
				progname);
		return 1;
	}
	while (wait(&status) != pid);
	return status;
}

/*
 * Write log entries.
 */
static void
dolog(int sign)
{
	FILE *fp;
	char *tty;
	const char format[] = "SU %02u/%02u %02u:%02u %c %s %s-%s\n";
	struct tm *tm;
	time_t t;

	time(&t);
	tm = localtime(&t);
	if ((tty = ttyname(0)) == NULL)
		tty = "/dev/???";
	if (dosyslog == LT_ALL || (dosyslog == LT_FAIL && sign == '-')) {
		openlog("su", LOG_PID, LOG_AUTH);
		if (sign == '-')
			syslog(LOG_CRIT | LOG_AUTH,
					"'su %s' failed for %s on %s",
				user, olduser, tty);
		else if (dosyslog != LT_FAIL)
			syslog((pwd->pw_uid ? LOG_INFO : LOG_NOTICE) | LOG_AUTH,
					"'su %s' succeeded for %s on %s",
				user, olduser, tty);
		closelog();
	}
	if (strncmp(tty, "/dev/", 5) == 0)
		tty += 5;
	if (sulog && (fp = fopen(sulog, "a+")) != NULL) {
		fprintf(fp, format, tm->tm_mon + 1, tm->tm_mday,
				tm->tm_hour, tm->tm_min,
				sign, tty,
				olduser, user);
		fclose(fp);
	}
	if (console && (fp = fopen(console, "a+")) != NULL) {
		fprintf(fp, format, tm->tm_mon + 1, tm->tm_mday,
				tm->tm_hour, tm->tm_min,
				sign, tty,
				olduser, user);
		fclose(fp);
	}
}

/*
 * Parse defaults file.
 */
static void
defaults(void)
{
	FILE *fp;

	if ((fp = fopen(defaultfile, "r")) != NULL) {
		char buf[LINE_MAX];
		char *cp;

		while (fgets(buf, sizeof buf, fp) != NULL) {
			if (buf[0] == '\0' || buf[0] == '\n' || buf[0] == '#')
				continue;
			if ((cp = strchr(buf, '\n')) != NULL)
				*cp = '\0';
			if (strncmp(buf, pwd->pw_uid ? "PATH=" : "SUPATH=",
					pwd->pw_uid ? 5 : 7) == 0) {
				path = smalloc(strlen(buf) + 1);
				strcpy(path, pwd->pw_uid ? buf : &buf[2]);
			} else if (strncmp(buf, "SULOG=", 6) == 0) {
				sulog = smalloc(strlen(buf) - 5);
				strcpy(sulog, &buf[6]);
			} else if (strncmp(buf, "CONSOLE=", 8) == 0) {
				console = smalloc(strlen(buf) - 7);
				strcpy(console, &buf[8]);
			} else if (strncmp(buf, "SLEEPTIME=", 10) == 0) {
				sleeptime = atoi(&buf[10]);
				if (sleeptime < 0 || sleeptime > 5)
					sleeptime = 4;
			} else if (strncmp(buf, "SYSLOG=", 7) == 0) {
				if (strcasecmp(&buf[7], "yes") == 0)
					dosyslog = LT_ALL;
				else if (strcasecmp(&buf[7], "fail") == 0)
					dosyslog = LT_FAIL;
			} else if (strncmp(buf, "DOFORK=", 7) == 0) {
				if (strcasecmp(&buf[7], "yes") == 0)
					dofork = 1;
			}
		}
		fclose(fp);
	}
	if (path == NULL)
		path = (char *)(pwd->pw_uid ? PATH : SUPATH);
}

/*
 * Ask for passwords.
 */
static char *
ask(const char *msg, int echo)
{
	struct termios tio;
	char *rsp;
	tcflag_t lflag;
	int fd, i = 0;
	char c;
	size_t sz;

	if ((fd = open("/dev/tty", O_RDWR)) < 0)
		fd = 0;
	if (tcgetattr(fd, &tio) < 0) {
		if (fd)
			close(fd);
		return NULL;
	}
	lflag = tio.c_lflag;
	if (echo == 0) {
		tio.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
		if (tcsetattr(fd, TCSAFLUSH, &tio) < 0) {
			if (fd)
				close(fd);
			return NULL;
		}
	}
	sz = strlen(msg);
	if (sz > 2 && msg[sz-1] == ' ' && msg[sz-2] == ':')
		sz--;
	fwrite(msg, sizeof *msg, sz, stderr);
	rsp = smalloc(PAM_MAX_RESP_SIZE + 1);
	while (read(fd, &c, 1) == 1 && c != '\n')
		if (i < PAM_MAX_RESP_SIZE)
			rsp[i++] = c;
	rsp[i] = '\0';
	fputc('\n', stderr);
	if (echo == 0) {
		tio.c_lflag = lflag;
		tcsetattr(fd, TCSADRAIN, &tio);
	}
	if (fd)
		close(fd);
	return rsp;
}

#ifdef	PAM
/*
 * PAM conversation.
 */
static int
doconv(int num_msg, const struct pam_message **msg,
		struct pam_response **rsp, void *appdata) {
	char *cp;
	int i;

	if (num_msg <= 0)
		return PAM_CONV_ERR;
	for (i = 0; i < num_msg; i++) {
		switch (msg[i]->msg_style) {
		case PAM_PROMPT_ECHO_ON:
		case PAM_PROMPT_ECHO_OFF:
			if ((cp = ask(msg[i]->msg, msg[i]->msg_style
					== PAM_PROMPT_ECHO_ON)) == NULL)
				return PAM_CONV_ERR;
			rsp[i] = smalloc(sizeof *rsp[i]);
			rsp[i]->resp = cp;
			rsp[i]->resp_retcode = 0;
			break;
		case PAM_ERROR_MSG:
			fprintf(stderr, "%s\n", msg[i]->msg);
			break;
		case PAM_TEXT_INFO:
			printf("%s\n", msg[i]->msg);
			break;
		}
	}
	return PAM_SUCCESS;
}

static struct pam_conv conv = {
	doconv,
	NULL
};
#else	/* !PAM */
/*
 * Check if the passed string is possibly a valid password, either a
 * traditional or a MD5 one.
 */
#ifdef	SHADOW_PWD
static int
cantbevalid(const char *ncrypt)
{
	if (ncrypt == NULL || strlen(ncrypt) < 13)
		return 1;
	while (*ncrypt)
		if (strchr(
	"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789./$",
				*ncrypt) == NULL)
			return 1;
	return 0;
}
#endif	/* SHADOW_PWD */

/*
 * Traditional password file authentication.
 */
static int
authenticate(const char *name, const char *ncrypt)
{
	char	*password;
	int	val;

#ifdef	SHADOW_PWD
	if (cantbevalid(ncrypt)) {
		struct spwd	*sp;
		time_t	now;

		if ((sp = getspnam(name)) == NULL)
			return 0;
		time(&now);
		if (sp->sp_expire > 0 && now >= sp->sp_expire)
			return 0;
		if (sp->sp_lstchg > 0 && sp->sp_max >= 0 && sp->sp_inact >= 0 &&
			    now >= sp->sp_lstchg + sp->sp_max + sp->sp_inact)
			return 0;
		ncrypt = sp->sp_pwdp;
	}
#endif	/* SHADOW_PWD */
	if (*ncrypt == '\0')
		return 1;
	if ((password = ask("Password:", 0)) == NULL)
		return 0;
	val = strcmp(ncrypt, crypt(password, ncrypt)) == 0;
	while (*password)
		*password++ = '\0';
	return val;
}
#endif	/* !PAM */

int
main(int argc, char **argv)
{
#ifdef	PAM
	pam_handle_t *pamh = NULL;
	int ret;
#endif	/* PAM */
	int status = 0;
	uid_t	myuid;

	progname = basename(argv[0]);
	oldint = signal(SIGINT, SIG_IGN);
	oldquit = signal(SIGQUIT, SIG_IGN);
	if (argc > 1 && argv[1][0] == '-') {
		dash++;
		argc--, argv++;
	}
	if (argc > 1) {
		user = argv[1];
		argc--, argv++;
	} else
		user = "root";
	myuid = getuid();
	if ((pwd = getpwuid(myuid)) == NULL) {
		fprintf(stderr, "%s: you do not exist\n", progname);
		exit(1);
	}
	olduser = smalloc(strlen(pwd->pw_name) + 1);
	strcpy(olduser, pwd->pw_name);
	if ((pwd = getpwnam(user)) == NULL) {
		fprintf(stderr, "%s: Unknown id: %s\n", progname, user);
		exit(1);
	}
	if (pwd->pw_dir != NULL && *pwd->pw_dir != '\0') {
		dir = smalloc(strlen(pwd->pw_dir) + 1);
		strcpy(dir, pwd->pw_dir);
	}
	if (pwd->pw_shell != NULL && *pwd->pw_shell != '\0') {
		shell = smalloc(strlen(pwd->pw_shell) + 1);
		strcpy(shell, pwd->pw_shell);
	} else
		shell = "/bin/sh";
	defaults();
#ifdef	PAM
	ret = pam_start("su", user, &conv, &pamh);
	if (ret == PAM_SUCCESS)
		ret = pam_authenticate(pamh, 0);
	if (ret == PAM_SUCCESS)
		ret = pam_acct_mgmt(pamh, 0);
	if (ret == PAM_SUCCESS) {
#else	/* PAM */
	if (myuid == 0 || authenticate(user, pwd->pw_passwd) != 0) {
#endif	/* !PAM */
		dolog('+');
		if (dofork) {
			status = dospawn(argv);
		} else {
#ifdef	PAM
			pam_end(pamh, ret);
#endif	/* PAM */
			child(argv);
			/*NOTREACHED*/
		}
	} else {
		dolog('-');
		if (sleeptime)
			sleep(sleeptime);
		fprintf(stderr, "%s: gomenasai~\n", progname);
		status = 1;
	}
#ifdef	PAM
	pam_end(pamh, ret);
#endif	/* PAM */
	return status;
}
