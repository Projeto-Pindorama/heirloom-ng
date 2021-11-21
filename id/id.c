/*
 * id - print user and group IDs and names
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, August 2002.
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
#ifdef	SUS
static const char sccsid[] USED = "@(#)id_sus.sl	1.10 (gritter) 5/29/05";
#else
static const char sccsid[] USED = "@(#)id.sl	1.10 (gritter) 5/29/05";
#endif

#include	<unistd.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<libgen.h>
#include	<pwd.h>
#include	<grp.h>

static int	aflag;			/* print supplementary GIDs */
static int	nflag;			/* print names instead of numbers */
static int	rflag;			/* use real IDs */
static int	restriction;		/* 'G' or 'g' or 'u' */
static char	*progname;		/* argv[0] to main() */

static void *
srealloc(void *vp, size_t nbytes)
{
	void	*p;

	if ((p = (void *)realloc(vp, nbytes)) == NULL) {
		write(2, "Out of memory\n", 14);
		exit(077);
	}
	return p;
}

static void *
smalloc(size_t nbytes)
{
	return srealloc(NULL, nbytes);
}

static void
usage(void)
{
#ifdef	SUS
	fprintf(stderr, "\
Usage: %s [user]\n\
       %s -a [user]\n\
       %s -G [-n] [user]\n\
       %s -g [-nr] [user]\n\
       %s -u [-nr] [user]\n",
		progname, progname, progname, progname, progname);
#else
	fprintf(stderr, "usage: %s [-a]\n", progname);
#endif
	exit(2);
}

static int	putspace;

static void
print_id(char *string, unsigned id, char *name)
{
	if (restriction) {
		if (putspace)
			putchar(' ');
		if (nflag)
			printf("%s", name);
		else
			printf("%u", id);
	} else {
		printf("%s%s=%u", putspace ? " " : "", string, id);
		if (name)
			printf("(%s)", name);
	}
	putspace++;
}

static void
print_supp(unsigned id, char *name)
{
	if (restriction) {
		if (putspace++)
			putchar(' ');
		if (nflag)
			printf("%s", name);
		else
			printf("%u", id);
	} else {
		static int	putcomma;

		if (putcomma++ == 0)
			printf(" groups=");
		else
			putchar(',');
		printf("%u", id);
		if (name)
			printf("(%s)", name);
	}
}

static void
supplementary(int me, char *name, gid_t mygid, gid_t myegid)
{
	struct group	*grp;
	gid_t	*groups;
	int	i, count;

	if (me) {
		if ((count = getgroups(0, NULL)) > 0) {

			groups = smalloc(count * sizeof *groups);
			getgroups(count, groups);
			for (i = 0; i < count; i++) {
				if (mygid != myegid || mygid != groups[i] ||
						aflag > 1)
				{
					grp = getgrgid(groups[i]);
					print_supp(groups[i], grp ?
						grp->gr_name : NULL);
				}
			}
			free(groups);
		}
	} else if (name) {
		setgrent();
		while ((grp = getgrent()) != NULL) {
			if (mygid != myegid || mygid != grp->gr_gid ||
					aflag > 1)
			{
				if (grp->gr_mem)
					for (i = 0; grp->gr_mem[i]; i++)
						if (strcmp(grp->gr_mem[i],
								name) == 0)
							print_supp(grp->gr_gid,
								grp->gr_name);
			}
		}
		endgrent();
	}
}

static void
id(int me, uid_t uid, uid_t euid, gid_t gid, gid_t egid)
{
	struct passwd	*pwd;
	struct group	*grp;
	char	*name;

	pwd = getpwuid(uid);
	if (restriction == 0 || (restriction == 'u' && rflag))
		print_id("uid", uid, pwd ? pwd->pw_name : NULL);
	if (pwd) {
		name = smalloc(strlen(pwd->pw_name) + 1);
		strcpy(name, pwd->pw_name);
	} else
		name = NULL;
	grp = getgrgid(gid);
	if (restriction == 0 || (restriction == 'g' && rflag) ||
			restriction == 'G')
		print_id("gid", gid, grp ? grp->gr_name : NULL);
	if ((restriction == 0 && uid != euid) ||
			(restriction == 'u' && rflag == 0)) {
		pwd = getpwuid(euid);
		print_id("euid", euid, pwd ? pwd->pw_name : NULL);
	}
	if (((restriction == 0 || restriction == 'G') && gid != egid) ||
			(restriction == 'g' && rflag == 0)) {
		grp = getgrgid(egid);
		print_id("egid", egid, grp ? grp->gr_name : NULL);
	}
	if ((restriction == 0 && aflag) || restriction == 'G')
		supplementary(me, name, gid, egid);
	putchar('\n');
}

int
main(int argc, char **argv)
{
#ifdef	SUS
	const char	optstring[] = "aGgnru";
#else
	const char	optstring[] = "a";
#endif
	int	i, me;
	uid_t	uid, euid;
	gid_t	gid, egid;

#ifdef	__GLIBC__
	putenv("POSIXLY_CORRECT=1");
#endif
	progname = basename(argv[0]);
#ifdef	SUS
	aflag = 1;
#endif
	while ((i = getopt(argc, argv, optstring)) != EOF) {
		switch (i) {
		case 'a':
			aflag = 2;
			break;
		case 'n':
			nflag = 1;
			break;
		case 'r':
			rflag = 1;
			break;
		case 'G':
		case 'g':
		case 'u':
			if (restriction)
				usage();
			restriction = i;
			break;
		default:
			usage();
		}
	}
	if (restriction == 0 && (nflag || rflag))
		usage();
	if (restriction != 0 && aflag > 1)
		usage();
	if (restriction == 'G' && rflag)
		usage();
#ifdef	SUS
	if (argc - optind == 1) {
		struct passwd	*pwd;

		if ((pwd = getpwnam(argv[optind])) == NULL) {
			fprintf(stderr, "%s: invalid user name: %s\n",
					progname, argv[optind]);
			exit(1);
		}
		me = 0;
		uid = euid = pwd->pw_uid;
		gid = egid = pwd->pw_gid;
	} else if (argc < optind > 1) {
		usage();
	} else
#endif	/* SUS */
	{
		me = 1;
		uid = getuid();
		euid = geteuid();
		gid = getgid();
		egid = getegid();
	}
	id(me, uid, euid, gid, egid);
	return 0;
}
