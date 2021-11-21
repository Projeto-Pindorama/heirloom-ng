/*
 * paste - merge lines
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
static const char sccsid[] USED = "@(#)paste.sl	1.11 (gritter) 5/29/05";

#include	<unistd.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<libgen.h>
#include	<limits.h>
#include	<wchar.h>
#include	<locale.h>

#include	<mbtowi.h>

struct	file {
	struct file	*f_nxt;
	const char	*f_fn;
	FILE	*f_fp;
};

struct	delim {
	struct delim	*d_nxt;
	wint_t	d_wc;
	char	d_mb[MB_LEN_MAX + 1];
};

unsigned	errcnt;			/* count of errors */
int		sflag;			/* paste files serially */
char		*progname;		/* argv[0] to main() */
struct delim	*dstart;		/* start of delimiters */
struct delim	*dcur;			/* current delimiter */

#ifdef	__GLIBC__
#ifdef	_IO_getc_unlocked
#undef	getc
#define	getc(f)		_IO_getc_unlocked(f)
#endif	/* _IO_getc_unlocked */
#ifdef	_IO_putc_unlocked
#undef	putchar
#define	putchar(c)	_IO_putc_unlocked(c, stdout)
#endif	/* _IO_putc_unlocked */
#endif	/* __GLIBC__ */

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
	fprintf(stderr,
		"Usage: %s [-s] [-d<delimiterstring>] file1 file2 ...\n",
		progname);
	exit(2);
}

static void
nodelim(void)
{
	fprintf(stderr, "%s: no delimiters\n", progname);
	exit(2);
}

static void
cantopen(const char *fn)
{
	fprintf(stderr, "%s: cannot open %s\n", progname, fn);
	if (sflag == 0)
		exit(1);
}

static void
setdelim(const char *s)
{
	const char	*sp;
	struct delim	*dp, *dq = NULL;
	wint_t	wc;
	int	i, n;

	while ((n = mbtowi(&wc, s, MB_LEN_MAX)) != 0) {
		if (n < 0) {
			wc = WEOF;
			n = 1;
		}
		if (wc == '\\') {
			s += n;
			if ((n = mbtowi(&wc, s, MB_LEN_MAX)) == 0)
				break;
			if (n < 0) {
				wc = WEOF;
				n = 1;
			}
			switch (wc) {
			case 'n':
				wc = '\n';
				sp = "\n";
				break;
			case 't':
				wc = '\t';
				sp = "\t";
				break;
			case '0':
				wc = '\0';
				sp = "\0";
				break;
			/* '\\' is handled by default code */
			default:
				sp = s;
			}
		} else
			sp = s;
		dp = smalloc(sizeof *dp);
		dp->d_wc = wc;
		for (i = 0; i < n && i < sizeof dp->d_mb - 1; i++)
			dp->d_mb[i] = sp[i];
		dp->d_mb[i] = '\0';
		if (dq)
			dq->d_nxt = dp;
		else
			dstart = dp;
		dq = dp;
		s += n;
	}
	if (dstart == NULL)
		nodelim();
	dq->d_nxt = dstart;
}

static void
resdelim(void)
{
	dcur = dstart;
}

static void
putdelim(void)
{
	const char	*cp;

	if (dcur) {
		if (dcur->d_wc)
			for (cp = dcur->d_mb; *cp; cp++)
				putchar(*cp);
		dcur = dcur->d_nxt;
	} else
		putchar('\t');
}

static int
serial(const char *fn)
{
	FILE	*fp;
	int	c;

	resdelim();
	if (fn[0] == '-' && fn[1] == '\0')
		fp = stdin;
	else if ((fp = fopen(fn, "r")) == NULL) {
		cantopen(fn);
		return 1;
	}
	c = getc(fp);
	while (c != EOF) {
		if (c == '\n') {
			if ((c = getc(fp)) != EOF)
				putdelim();
		} else {
			putchar(c);
			c = getc(fp);
		}
	}
	putchar('\n');
	if (fp != stdin)
		fclose(fp);
	return 0;
}

static int
paste(char **args)
{
	struct file	*fstart = NULL, *fp, *fq = NULL;
	int	c, i, oneof, hadnl, delcnt;

	for (i = 0; args[i]; i++) {
		fp = smalloc(sizeof *fp);
		fp->f_nxt = NULL;
		fp->f_fn = args[i];
		if (fp->f_fn[0] == '-' && fp->f_fn[1] == '\0')
			fp->f_fp = stdin;
		else if ((fp->f_fp = fopen(fp->f_fn, "r")) == NULL)
			cantopen(fp->f_fn);
		if (fq)
			fq->f_nxt = fp;
		else
			fstart = fp;
		fq = fp;
	}
	do {
		oneof = 1;
		hadnl = 0;
		delcnt = 0;
		resdelim();
		for (fp = fstart; fp; fp = fp->f_nxt) {
			while ((c = getc(fp->f_fp)) != EOF && c != '\n') {
				while (delcnt > 0) {
					putdelim();
					delcnt--;
				}
				putchar(c);
			}
			if (c == '\n')
				hadnl = 1;
			if (c != EOF && (c = getc(fp->f_fp)) != EOF)
				ungetc(c, fp->f_fp);
			if (c != EOF)
				oneof = 0;
			if (fp->f_nxt) {
				if (oneof)
					delcnt++;
				else
					putdelim();
			} else if (hadnl) {
				while (delcnt > 0) {
					putdelim();
					delcnt--;
				}
				putchar('\n');
			}
		}
	} while (oneof == 0);
	for (fp = fstart; fp; fp = fp->f_nxt)
		if (fp->f_fp != stdin)
			fclose(fp->f_fp);
	return 0;
}

int
main(int argc, char **argv)
{
	const char	optstring[] = ":d:s";
	int	i;

#ifdef	__GLIBC__
	putenv("POSIXLY_CORRECT=1");
#endif
	progname = basename(argv[0]);
	setlocale(LC_CTYPE, "");
	while ((i = getopt(argc, argv, optstring)) != EOF) {
		switch (i) {
		case 'd':
			setdelim(optarg);
			break;
		case 's':
			sflag = 1;
			break;
		case ':':
			nodelim();
		default:
			usage();
		}
	}
	if (sflag) {
		for (i = optind; i < argc; i++)
			errcnt |= serial(argv[i]);
	} else if (argc)
		errcnt |= paste(&argv[optind]);
	return errcnt;
}
