/*
 * news - print news items
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, December 2002.
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
static const char sccsid[] USED = "@(#)news.sl	1.11 (gritter) 5/29/05";

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<dirent.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<time.h>
#include	<libgen.h>
#include	<utime.h>
#include	<pwd.h>
#include	<setjmp.h>
#include	<signal.h>
#include	"sigset.h"

#include	"iblok.h"
#include	"oblok.h"

struct	item {
	const char	*i_name;
	time_t		i_time;
	uid_t		i_uid;
};

static const char	newsdir[] = "/var/news";
static const char	currency[] = ".news_time";

static unsigned		errcnt;		/* count of errors */
static int		aflag;		/* print all items */
static int		nflag;		/* print names of items only */
static int		sflag;		/* print item count */
static char		*progname;	/* argv[0] to main() */
static unsigned long	item_count;	/* count of items */
static time_t		news_time;	/* time of last news printing */
static time_t		start_time;	/* time at start of command */
static int		canjump;	/* jmpenv is valid */
static jmp_buf		jmpenv;		/* jump buffer */
static struct oblok	*output;	/* output buffer */

/*
 * Memory allocation with check.
 */
void *
srealloc(void *vp, size_t nbytes)
{
	void	*p;

	if ((p = (void *)realloc(vp, nbytes)) == NULL) {
		write(2, "No storage\n", 11);
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
	fprintf(stderr, "usage: %s [-a] [-n] [-s] [items]\n", progname);
	exit(2);
}

static char *
components(const char *prefix, const char *name)
{
	char	*cp;

	if (prefix) {
		if (name) {
			cp = smalloc(strlen(prefix) + strlen(name) + 2);
			strcpy(cp, prefix);
			strcat(cp, "/");
			strcat(cp, name);
		} else {
			cp = smalloc(strlen(prefix) + 1);
			strcpy(cp, prefix);
		}
	} else {
		cp = smalloc(strlen(name) + 1);
		strcpy(cp, name);
	}
	return cp;
}

static void
cantopen(const char *name)
{
	char	*file;

	file = components(newsdir, name);
	fprintf(stderr, "Cannot open %s\n", file);
	free(file);
	errcnt |= 1;
}

static void
onint(int signo)
{
	static time_t	cur, last;

	time(&cur);
	if (cur - last <= 1)
		exit(0200 + signo);
	if (canjump)
		longjmp(jmpenv, signo);
}

static void
cat(int fd)
{
	struct iblok	*ip;
	int	signo;
	int	c, lastc = '\n';

#ifdef	__GNUC__
	(void)&lastc;
#endif	/* __GNUC__ */
	ip = ib_alloc(fd, 0);
	if ((signo = setjmp(jmpenv)) == 0) {
		canjump = 1;
		while ((c = ib_get(ip)) != EOF) {
			if (lastc == '\n')
				ob_write(output, "   ", 3);
			ob_put(c, output);
			lastc = c;
		}
		canjump = 0;
	} else
		sigrelse(signo);
	ib_free(ip);
}

static char *
user(uid_t uid)
{
	struct passwd	*pw;
	char	*cp;
	size_t sz;

	if ((pw = getpwuid(uid)) != NULL) {
		cp = smalloc(sz = strlen(pw->pw_name) + 3);
		snprintf(cp, sz, "(%s)", pw->pw_name);
	} else {
		const char	dots[] = ".....";

		cp = smalloc(strlen(dots) + 1);
		strcpy(cp, dots);
	}
	return cp;
}

static void
item(const struct item *ip)
{
	if (nflag) {
		if (item_count++ == 0)
			printf("news:");
		printf(" %s", ip->i_name);
	} else if (!sflag) {
		char	*cp, *up;
		int	fd, n, sz;

		if ((fd = open(ip->i_name, O_RDONLY)) >= 0) {
			up = user(ip->i_uid);
			cp = malloc(sz = strlen(ip->i_name)+strlen(up)+30);
			n = snprintf(cp, sz, "%s%s %s %s",
				item_count ? "" : "\n",
				ip->i_name, up, ctime(&ip->i_time));
			ob_write(output, cp, n);
			free(cp);
			free(up);
			cat(fd);
			close(fd);
			ob_write(output, "\n", 1);
			item_count++;
		} else
			cantopen(ip->i_name);
	} else
		item_count++;
}

static time_t
gettime(const char *prefix, const char *name, uid_t *uid)
{
	struct stat	st;
	char	*file;
	time_t	t;

	file = components(prefix, name);
	if (stat(file, &st) == 0) {
		if (uid)
			*uid = st.st_uid;
		t = st.st_mtime;
	} else
		t = 0;
	free(file);
	return t;
}

static void
touch(const char *prefix, const char *name)
{
	struct stat	st;
	struct utimbuf	ut;
	char	*file;

	file = components(prefix, name);
	if (stat(file, &st) < 0) {
		close(open(file, O_WRONLY|O_CREAT, 0666));
	} else {
		ut.actime = st.st_atime;
		ut.modtime = start_time;
		utime(file, &ut);
	}
	free(file);
}

static int
bytime(const void *v1, const void *v2)
{
	return ((*(struct item **)v2)->i_time - (*(struct item **)v1)->i_time);
}

static void
allnews(void)
{
	DIR	*Dp;
	struct dirent	*dp;
	struct item	**ip = NULL;
	int	c = 0, i;

	if ((Dp = opendir(".")) == NULL) {
		cantopen(NULL);
		exit(1);
	}
	while ((dp = readdir(Dp)) != NULL) {
		if ((dp->d_name[0] == '.' && (dp->d_name[1] == '\0' ||
		    (dp->d_name[1] == '.' && dp->d_name[2] == '\0'))) ||
				strcmp(dp->d_name, "core") == 0)
			continue;
		ip = srealloc(ip, (c + 1) * sizeof *ip);
		ip[c] = smalloc(sizeof **ip);
		ip[c]->i_name = strdup(dp->d_name);
		ip[c]->i_time = gettime(NULL, ip[c]->i_name, &ip[c]->i_uid);
		c++;
	}
	if (ip == NULL)
		return;
	qsort(ip, c, sizeof *ip, bytime);
	for (i = 0; i < c && (aflag || ip[i]->i_time > news_time); i++)
		item(ip[i]);
}

int
main(int argc, char **argv)
{
	const char	optstring[] = "ans";
	const char	*home;
	int	i;

	progname = basename(argv[0]);
	while ((i = getopt(argc, argv, optstring)) != EOF) {
		switch (i) {
		case 'a':
			aflag = 1;
			break;
		case 'n':
			nflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		default:
			usage();
		}
	}
	if (aflag + nflag + sflag > 1) {
		fprintf(stderr, "%s: options are mutually exclusive\n",
				progname);
		exit(2);
	}
	if ((aflag || nflag || sflag) && argv[optind]) {
		fprintf(stderr, "%s: options are not allowed with file names\n",
				progname);
		exit(2);
	}
	if (chdir(newsdir) < 0) {
		fprintf(stderr, "Cannot chdir to %s\n", newsdir);
		exit(1);
	}
	if ((home = getenv("HOME")) == NULL) {
		fprintf(stderr, "Cannot find HOME variable\n");
		exit(1);
	}
	output = ob_alloc(1, OB_EBF);
	if (!aflag && !nflag && !sflag) {
		if (sigset(SIGINT, SIG_IGN) != SIG_IGN)
			sigset(SIGINT, onint);
	}
	news_time = gettime(home, currency, NULL);
	if (optind != argc) {
		struct item	it;

		for (i = optind; i < argc; i++) {
			it.i_name = argv[i];
			it.i_time = gettime(NULL, it.i_name, &it.i_uid);
			item(&it);
		}
	} else {
		time(&start_time);
		allnews();
		if (!aflag && !nflag && !sflag && item_count > 0)
			touch(home, currency);
	}
	if (nflag) {
		if (item_count > 0)
			printf("\n");
	} else if (sflag) {
		if (item_count > 0)
			printf("%lu news item%s.\n", item_count,
					item_count > 1 ? "s" : "");
		else
			printf("No news.\n");
	}
	return errcnt;
}

/*ARGUSED*/
void
writerr(struct oblok *op, int count, int written)
{
}
