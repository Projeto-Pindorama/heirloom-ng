/*
 * logins - list login information
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, December 2000.
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
static const char sccsid[] USED = "@(#)logins.sl	1.24 (gritter) 5/29/05";

#include	"config.h"
#include	<sys/types.h>
#include	<unistd.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<time.h>
#include	<pwd.h>
#ifdef	SHADOW_PWD
#include	<shadow.h>
#endif	/* SHADOW_PWD */
#include	<grp.h>
#include	<libgen.h>

#define	MIN_USER_UID	100
#define	DEFAULT_SH	"/bin/sh"

struct grp {
	struct grp	*g_next;	/* next ptr in table */
	char		*g_name;	/* group name */
	gid_t		g_gid;		/* gid */
};

#if defined (__hpux)
#define	user	user__
#endif	/* __hpux */

struct user {
	struct user	*u_next;	/* next ptr in table */
	char		*u_name;	/* user name */
	uid_t		u_uid;		/* uid */
};

struct login {
	long		l_lstchg;	/* date of last change */
	long		l_min;		/* min days to pwd change */
	long		l_max;		/* max days to pwd change */
	long		l_warn;		/* warning period */
	long		l_inact;	/* max days inactive */
	long		l_expire;	/* expiry date */
	unsigned long	l_flag;		/* unused flag */
	struct login	*l_next;	/* next ptr in table */
	char		*l_gecos;	/* gecos information */
	char		*l_dir;		/* home directory */
	char		*l_shell;	/* login shell */
	char		*l_pwdp;	/* encrypted pwd */
	char 		*l_name;	/* user name */
	struct grp	*l_grp;		/* group information */
	uid_t		l_uid;		/* uid */
};

unsigned	errcnt;			/* count of errors */
int		aflag;			/* show pwd expiration */
int		dflag;			/* duplicate uids */
int		mflag;			/* multiple group membership */
int		oflag;			/* comma-separated output */
int		pflag;			/* no pwd */
int		sflag;			/* system logins only */
int		tflag;			/* sort by login name */
int		uflag;			/* user logins only */
int		xflag;			/* extended output format */
struct grp	*selgrp;		/* selected groups only */
struct user	*seluser;		/* selected usernames only */
struct login	*l0;			/* start of logins table */
char		*progname;		/* program's name */

/*
 * Memory allocation with check.
 */
void *
salloc(size_t nbytes)
{
	char *p;

	if ((p = calloc(nbytes, sizeof *p)) == NULL) {
		write(2,
		"Internal space allocation error.  Command terminates\n", 53);
		exit(077);
	}
	return p;
}

/*
 * Return a password type string.
 */
char *
pwstr(const char *pwd)
{
	if (pwd != NULL) {
		if (*pwd == '\0')
			return "NP";
		if (strlen(pwd) > 12) {
			const char *cp;

			for (cp = pwd; *cp; cp++)
				/*
				 * Valid encrypted passwords consist of
				 * the characters in [a-zA-Z./]. A MD5
				 * encrypted password on Linux may also
				 * contain [$].
				 */
				if (strchr("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789./$", *cp) == NULL)
					goto locked;
			return "PS";
		}
	}
locked:
	return "LK";
}

/*
 * Single login output.
 */
void
out(const struct login *lp)
{
	struct grp *gp;

	printf(oflag ? "%s:%u:%s:%u:%s" : "%-16s %-6u %-16s %-6u %s\n",
			lp->l_name, (int)lp->l_uid,
			lp->l_grp->g_name, lp->l_grp->g_gid,
			lp->l_gecos);
	if (xflag) {
		time_t tchg;
		char datebuf[7];

		tchg = lp->l_lstchg * 86400;
		strftime(datebuf, sizeof datebuf, "%m%d%y", localtime(&tchg));
		printf(oflag ? ":%s:%s:%s:%s:%ld:%ld:%ld" :
				"                        %s\n\
                        %s\n\
                        %s %s %ld %ld %ld\n",
				lp->l_dir, lp->l_shell,
				pwstr(lp->l_pwdp), datebuf,
				lp->l_min, lp->l_max, lp->l_warn);
	}
	if (aflag) {
		printf(oflag ? ":%ld:%ld" :
				"                        %ld %06ld\n",
				lp->l_inact, lp->l_expire);
	} else if (mflag) {
		for (gp = lp->l_grp->g_next; gp; gp = gp->g_next)
			printf(oflag ? ":%s:%u" :
					"                        %-16s %-6u\n",
					gp->g_name, (int)gp->g_gid);
	}
	if (oflag)
		printf("\n");
}

/*
 * Sort by login name.
 */
int
loginsort(const void *a, const void *b)
{
	return strcmp((*(struct login **)a)->l_name,
			(*(struct login **)b)->l_name);
}

/*
 * Sort by user id.
 */
int
uidsort(const void *a, const void *b)
{
	return ((*(struct login **)a)->l_uid - (*(struct login **)b)->l_uid);
}

/*
 * Sort and report the logins table.
 */
void
report(void)
{
	int i, lc;
	struct login **lb, *lp;

	if (l0 == NULL)
		return;
	for (lc = 1, lp = l0; lp->l_next; lp = lp->l_next, lc++);
	lb = (struct login **)salloc(sizeof *lb * lc);
	for (i = 0, lp = l0; i < lc; i++, lp = lp->l_next)
		lb[i] = lp;
	qsort(lb, lc, sizeof *lb, tflag ? loginsort : uidsort);
	for (i = 0; i < lc; i++)
		out(lb[i]);
}

/*
 * Check whether a login has a duplicate uid.
 */
int
duplicate(const struct login *lp)
{
	struct login *l;

	for (l = l0; l; l = l->l_next)
		if (lp->l_uid == l->l_uid && strcmp(lp->l_name, l->l_name))
			return 1;
	return 0;
}

/*
 * Check whether a login is in the selected groups table.
 */
int
ingroup(const struct login *lp)
{
	struct grp *gp, *glp;

	if (selgrp == NULL)
		return 1;
	for (gp = selgrp; gp; gp = gp->g_next)
		for (glp = lp->l_grp; glp; glp = glp->g_next)
			if (gp->g_gid == glp->g_gid)
				return 1;
	return 0;
}

/*
 * Delete logins according to the criteria given in options.
 */
void
userdel(void)
{
	struct login *lp, *lprev = NULL;

	if (dflag == 0 && pflag == 0 && selgrp == NULL)
		return;
	for (lp = l0; lp; lp = lp->l_next)
		if (pflag && (lp->l_pwdp == NULL || *lp->l_pwdp != '\0')
				|| selgrp && !ingroup(lp)
				|| dflag && !duplicate(lp)) {
			if (lprev != NULL)
				lprev->l_next = lp->l_next;
			else
				l0 = lp->l_next;
		} else
			lprev = lp;
}

/*
 * Fill group information to group table.
 */
void
groupgroup(const char *name, gid_t gid)
{
	struct grp *gp;

	for (gp = selgrp; gp; gp = gp->g_next)
		if (strcmp(gp->g_name, name) == 0)
			gp->g_gid = gid;
}

/*
 * Look if a string is contained in the given array.
 */
int
inarray(const char *str, char **arr)
{
	unsigned i = 0;

	while (arr[i])
		if (strcmp(arr[i++], str) == 0)
			return 1;
	return 0;
}

/*
 * Put group information in user tables.
 */
void
usergroup(const char *name, gid_t gid, char **mem)
{
	struct login *lp;
	struct grp *gp;
	size_t sz;

	for (lp = l0; lp; lp = lp->l_next) {
		sz = strlen(name);
		if (lp->l_grp->g_gid == gid) {
			lp->l_grp->g_name = salloc(sz + 1);
			strcpy(lp->l_grp->g_name, name);
		}
		if (inarray(lp->l_name, mem)) {
			for (gp = lp->l_grp; gp->g_next; gp = gp->g_next);
			gp->g_next = (struct grp *)salloc(sizeof *gp);
			gp = gp->g_next;
			gp->g_next = NULL;
			gp->g_name = salloc(sz + 1);
			strcpy(gp->g_name, name);
			gp->g_gid = gid;
		}
	}
}

/*
 * Read /etc/group.
 */
void
readgroup(void)
{
	struct group *gr;

	setgrent();
	while (gr = getgrent()) {
		groupgroup(gr->gr_name, gr->gr_gid);
		usergroup(gr->gr_name, gr->gr_gid, gr->gr_mem);
	}
	endgrent();
}

/*
 * Find the login entry that matches name.
 */
struct login *
findlogin(const char *name)
{
	struct login *lp;

	for (lp = l0; lp; lp = lp->l_next)
		if (strcmp(lp->l_name, name) == 0)
			return lp;
	return NULL;
}

/*
 * Read /etc/shadow.
 */
void
readshadow(void)
{
#ifdef	SHADOW_PWD
	struct login *lp;
	struct spwd *sp;
	size_t sz;

	setspent();
	while (sp = getspent()) {
		if ((lp = findlogin(sp->sp_namp)) == NULL)
			continue;
		sz = strlen(sp->sp_pwdp);
		lp->l_pwdp = salloc(sz + 1);
		strcpy(lp->l_pwdp, sp->sp_pwdp);
		lp->l_lstchg = sp->sp_lstchg;
		lp->l_min = sp->sp_min;
		lp->l_max = sp->sp_max;
		lp->l_warn = sp->sp_warn;
		lp->l_inact = sp->sp_inact;
		lp->l_expire = sp->sp_expire;
		lp->l_flag = sp->sp_flag;
	}
	endspent();
#endif	/* SHADOW_PWD */
}

/*
 * See if the specified user is to be excluded in output.
 */
int
excludeuser(const char *name, uid_t uid)
{
	struct user *up = seluser;

	if (uflag && uid < MIN_USER_UID)
		return 1;
	if (sflag && uid >= MIN_USER_UID)
		return 1;
	if (seluser == NULL)
		return 0;
	do
		if (strcmp(name, up->u_name) == 0) {
			up->u_uid = uid;
			return 0;
		}
	while (up = up->u_next);
	return 1;
}

/*
 * Read /etc/passwd.
 */
void
readpasswd(void)
{
	struct login *lp = NULL, *ln;
	struct passwd *pw;
	size_t sz;

	setpwent();
	while (pw = getpwent()) {
		if (excludeuser(pw->pw_name, pw->pw_uid))
			continue;
		ln = (struct login *)salloc(sizeof *ln);
		if (l0 == NULL)
			l0 = lp = ln;
		else {
			lp->l_next = ln;
			lp = ln;
		}
		sz = strlen(pw->pw_name);
		lp->l_name = salloc(sz + 1);
		strcpy(lp->l_name, pw->pw_name);
		lp->l_uid = pw->pw_uid;
		lp->l_grp = (struct grp *)salloc(sizeof *lp->l_grp);
		lp->l_grp->g_gid = pw->pw_gid;
		sz = strlen(pw->pw_gecos);
		lp->l_gecos = salloc(sz + 1);
		strcpy(lp->l_gecos, pw->pw_gecos);
		sz = strlen(pw->pw_dir);
		lp->l_dir = salloc(sz + 1);
		strcpy(lp->l_dir, pw->pw_dir);
		if (sz = strlen(pw->pw_shell)) {
			lp->l_shell = salloc(sz + 1);
			strcpy(lp->l_shell, pw->pw_shell);
		} else
			lp->l_shell = DEFAULT_SH;
	}
	if (lp)
		lp->l_next = NULL;
	endpwent();
}

/*
 * Put group selection information in internal format.
 */
void
selectgrp(const char *str)
{
	struct grp *gp;
	size_t sz;
	const char *endstr;

	gp = selgrp = (struct grp *)salloc(sizeof *gp);
	for (;;) {
		gp->g_gid = -1;
		if ((endstr = strchr(str, ',')) == NULL)
			endstr = str + strlen(str);
		sz = endstr - str;
		gp->g_name = salloc(sz + 1);
		memcpy(gp->g_name, str, sz);
		gp->g_name[sz] = '\0';
		if (getgrnam(gp->g_name) == NULL) {
			fprintf(stderr, "%s was not found\n", gp->g_name);
			exit(1);
		}
		if (*endstr == '\0' || *(str = endstr + 1) == '\0')
			break;
		str = endstr + 1;
		gp->g_next = (struct grp *)salloc(sizeof *gp);
		gp = gp->g_next;
	}
	gp->g_next = NULL;
}

/*
 * Put user selection information in internal format.
 */
void
selectuser(const char *str)
{
	struct user *up;
	size_t sz;
	const char *endstr;

	up = seluser = (struct user *)salloc(sizeof *up);
	for (;;) {
		up->u_uid = -1;
		if ((endstr = strchr(str, ',')) == NULL)
			endstr = str + strlen(str);
		sz = endstr - str;
		up->u_name = salloc(sz + 1);
		memcpy(up->u_name, str, sz);
		up->u_name[sz] = '\0';
		if (getpwnam(up->u_name) == NULL) {
			fprintf(stderr, "%s was not found\n", up->u_name);
			exit(1);
		}
		if (*endstr == '\0' || *(str = endstr + 1) == '\0')
			break;
		up->u_next = (struct user *)salloc(sizeof *up);
		up = up->u_next;
	}
	up->u_next = NULL;
}

void
usage(void)
{
	fprintf(stderr, "usage: %s [-admopstux] [-g groups] [-l logins]\n",
		progname);
	exit(2);
}

int
main(int argc, char **argv)
{
	const char optstr[] = ":admopstuxg:l:";
	int i;

#ifdef	__GLIBC__
	putenv("POSIXLY_CORRECT=1");
#endif
	progname = basename(argv[0]);
	while ((i = getopt(argc, argv, optstr)) != EOF) {
		switch (i) {
		case 'a':
			aflag = 1;
			break;
		case 'd':
			dflag = 1;
			break;
		case 'm':
			mflag = 1;
			break;
		case 'o':
			oflag = 1;
			break;
		case 'p':
			pflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		case 't':
			tflag = 1;
			break;
		case 'u':
			uflag = 1;
			break;
		case 'x':
			xflag = 1;
			break;
		case 'g':
			selectgrp(optarg);
			break;
		case 'l':
			selectuser(optarg);
			break;
		default:
			usage();
		}
	}
	if (argv[optind])
		usage();
	if (sflag && uflag)
		sflag = uflag = 0;
	readpasswd();
	readshadow();
	readgroup();
	userdel();
	report();
	return errcnt;
}
