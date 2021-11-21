/*
 * mkdir - make directories
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, March 2003.
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
static const char sccsid[] USED = "@(#)mkdir_sus.sl	1.7 (gritter) 5/29/05";
#else
static const char sccsid[] USED = "@(#)mkdir.sl	1.7 (gritter) 5/29/05";
#endif

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<libgen.h>
#include	<errno.h>

enum	okay {
	OKAY = 0,
	STOP = 1
};

static char	*progname;		/* argv[0] to main() */
static int	errcnt;			/* error status */
static char	*mflag;			/* set a mode */
static int	pflag;			/* create parent directories */
static mode_t	um;			/* umask value */

static mode_t	newmode(const char *ms, const mode_t pm, const char *fn);

static void
usage(void)
{
	fprintf(stderr, "%s: usage: %s [-m mode] [-p] dirname ...\n",
			progname, progname);
	exit(2);
}

static enum okay
makedir(const char *dir, int setmode, int intermediate)
{
	struct stat	st;
	mode_t	mode;
	int	err, sgid_bit;

	mode = mflag ? newmode(mflag, 0777, dir) : 0777;
	if (mkdir(dir, mode) < 0) {
		err = errno;
		/*
		 * When mkdir() fails for an existing directory that
		 * is a NFS mount point, Solaris indicates ENOSYS
		 * instead of EEXIST.
		 */
		if (pflag && (errno == EEXIST || errno == ENOSYS) &&
				(intermediate || stat(dir, &st) == 0 &&
				 (st.st_mode&S_IFMT) == S_IFDIR))
			return OKAY;
		fprintf(stderr, pflag ? "%s: \"%s\": %s\n" :
			"%s:  Failed to make directory \"%s\"; %s\n",
				progname, dir, strerror(err));
		errcnt++;
		return STOP;
	}
	if (mflag && setmode) {
		sgid_bit = stat(dir, &st) == 0 && st.st_mode & S_ISGID ?
			S_ISGID : 0;
		if (sgid_bit)
			mode = newmode(mflag, 0777 | sgid_bit, dir);
		chmod(dir, mode);
	}
#ifdef	SUS
	else if (intermediate) {
		sgid_bit = stat(dir, &st) == 0 && st.st_mode & S_ISGID ?
			S_ISGID : 0;
		chmod(dir, 0777 & ~(mode_t)um | 0300 | sgid_bit);
	}
#endif	/* SUS */
	return OKAY;
}

static void
parents(char *dir)
{
	char	*slash;
	char	c;

	slash = dir;
	do {
		while (*slash == '/')
			slash++;
		while (*slash != '/' && *slash != '\0')
			slash++;
		c = *slash;
		*slash = '\0';
		if (makedir(dir,
#ifdef	SUS
				c == '\0'
#else
				1
#endif
			   , c != '\0') != OKAY)
			return;
		*slash = c;
	} while (c != '\0');
}

int
main(int argc, char **argv)
{
	const char	optstring[] = "m:p";
	int	i;

	progname = basename(argv[0]);
#ifdef	SUS
	umask(um = umask(0));
#endif
	while ((i = getopt(argc, argv, optstring)) != EOF) {
		switch (i) {
		case 'm':
			mflag = optarg;
			newmode(mflag, 0777, NULL);
			break;
		case 'p':
			pflag = 1;
			break;
		default:
			usage();
		}
	}
	if (optind == argc)
		usage();
	for (i = optind; i < argc; i++)
		if (pflag)
			parents(argv[i]);
		else
			makedir(argv[i], 1, 0);
	return errcnt > 255 ? 255 : errcnt;
}

/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, March 2003.
 */
/*	from Unix 7th Edition /usr/src/cmd/chmod.c	*/
/*
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *   Redistributions of source code and documentation must retain the
 *    above copyright notice, this list of conditions and the following
 *    disclaimer.
 *   Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *   All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed or owned by Caldera
 *      International, Inc.
 *   Neither the name of Caldera International, Inc. nor the names of
 *    other contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * USE OF THE SOFTWARE PROVIDED FOR UNDER THIS LICENSE BY CALDERA
 * INTERNATIONAL, INC. AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL CALDERA INTERNATIONAL, INC. BE
 * LIABLE FOR ANY DIRECT, INDIRECT INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*	Sccsid @(#)modes.c	1.1 (gritter) 3/26/03	*/

#define	USER	05700	/* user's bits */
#define	GROUP	02070	/* group's bits */
#define	OTHER	00007	/* other's bits */
#define	ALL	07777	/* all */

#define	READ	00444	/* read permit */
#define	WRITE	00222	/* write permit */
#define	EXEC	00111	/* exec permit */
#define	SETID	06000	/* set[ug]id */
#define	STICKY	01000	/* sticky bit */

#ifndef	S_ENFMT
#define	S_ENFMT	02000	/* mandatory locking bit */
#endif

static mode_t	absol(const char **);
static mode_t	who(const char **, mode_t *);
static int	what(const char **);
static mode_t	where(const char **, mode_t, int *, int *, const mode_t);

static mode_t
newmode(const char *ms, const mode_t pm, const char *fn)
{
	register mode_t	o, m, b;
	int	lock, setsgid = 0, cleared = 0, copy = 0;
	mode_t	nm, om, mm;

	nm = om = pm;
	m = absol(&ms);
	if (!*ms) {
		nm = m;
		goto out;
	}
	if ((lock = (nm&S_IFMT) != S_IFDIR && (nm&(S_ENFMT|S_IXGRP)) == S_ENFMT)
			== 01)
		nm &= ~(mode_t)S_ENFMT;
	do {
		m = who(&ms, &mm);
		while (o = what(&ms)) {
			b = where(&ms, nm, &lock, &copy, pm);
			switch (o) {
			case '+':
				nm |= b & m & ~mm;
				if (b & S_ISGID)
					setsgid = 1;
				if (lock & 04)
					lock |= 02;
				break;
			case '-':
				nm &= ~(b & m & ~mm);
				if (b & S_ISGID)
					setsgid = 1;
				if (lock & 04)
					lock = 0;
				break;
			case '=':
				nm &= ~m;
				nm |= b & m & ~mm;
				lock &= ~01;
				if (lock & 04)
					lock |= 02;
				om = 0;
				if (copy == 0)
					cleared = 1;
				break;
			}
			lock &= ~04;
		}
	} while (*ms++ == ',');
	if (*--ms) {
		fprintf(stderr, "Invalid mode.\n");
		exit(2);
	}
out:	if ((pm & S_ISGID) && setsgid == 0)
		nm |= S_ISGID;
	else if ((nm & S_ISGID) && setsgid == 0)
		nm &= ~(mode_t)S_ISGID;
	return(nm);
}

static mode_t
absol(const char **ms)
{
	register int c, i;

	i = 0;
	while ((c = *(*ms)++) >= '0' && c <= '7')
		i = (i << 3) + (c - '0');
	(*ms)--;
	return(i);
}

static mode_t
who(const char **ms, mode_t *mp)
{
	register int m;

	m = 0;
	*mp = 0;
	for (;;) switch (*(*ms)++) {
	case 'u':
		m |= USER;
		continue;
	case 'g':
		m |= GROUP;
		continue;
	case 'o':
		m |= OTHER;
		continue;
	case 'a':
		m |= ALL;
		continue;
	default:
		(*ms)--;
		if (m == 0) {
			m = ALL;
			*mp = um;
		}
		return m;
	}
}

static int
what(const char **ms)
{
	switch (**ms) {
	case '+':
	case '-':
	case '=':
		return *(*ms)++;
	}
	return(0);
}

static mode_t
where(const char **ms, mode_t om, int *lock, int *copy, const mode_t pm)
{
	register mode_t m;

	m = 0;
	*copy = 0;
	switch (**ms) {
	case 'u':
		m = (om & USER) >> 6;
		goto dup;
	case 'g':
		m = (om & GROUP) >> 3;
		goto dup;
	case 'o':
		m = (om & OTHER);
	dup:
		*copy = 1;
		m &= (READ|WRITE|EXEC);
		m |= (m << 3) | (m << 6);
		++(*ms);
		return m;
	}
	for (;;) switch (*(*ms)++) {
	case 'r':
		m |= READ;
		continue;
	case 'w':
		m |= WRITE;
		continue;
	case 'x':
		m |= EXEC;
		continue;
	case 'X':
		if ((pm&S_IFMT) == S_IFDIR || (pm & EXEC))
			m |= EXEC;
		continue;
	case 'l':
		if ((pm&S_IFMT) != S_IFDIR)
			*lock |= 04;
		continue;
	case 's':
		m |= SETID;
		continue;
	case 't':
		m |= STICKY;
		continue;
	default:
		(*ms)--;
		return m;
	}
}
