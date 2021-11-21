/*
 * cat - concatenate files
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, July 2002.
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
static const char sccsid[] USED = "@(#)cat.sl	2.18 (gritter) 5/29/05";

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<libgen.h>
#include	<locale.h>
#include	<wctype.h>
#include	<ctype.h>

#include	"sfile.h"
#include	"iblok.h"
#include	"oblok.h"

static unsigned	errcnt;			/* count of errors */
static int	sflag;			/* silent about nonexistent files */
static int	uflag;			/* unbuffered output */
static int	vflag;			/* visible conversion */
static int	tflag;			/* tab conversion */
static int	eflag;			/* eol display */
static long	Bflag;			/* set buffer size, from SUPER-UX */
static char	*progname;		/* argv[0] to main() */
static struct stat	ostat;		/* output stat */
static int	multibyte;		/* MB_CUR_MAX > 1 */

void *
srealloc(void *ptr, size_t nbytes)
{
	void *p;

	if ((p = realloc(ptr, nbytes)) == NULL) {
		write(2, "No storage\n", 11);
		exit(077);
	}
	return p;
}

static void
usage(void)
{
	fprintf(stderr, "usage: %s -usvte [-|file] ...\n", progname);
	exit(2);
}

void
writerr(struct oblok *op, int count, int written)
{
	if (sflag == 0)
		fprintf(stderr,
			"%s: output error (%d/%d characters written)\n%s\n",
				progname, written, count,
				strerror(errno));
	errcnt |= 4;
}

static void
copy(const char *fn, struct oblok *op)
{
	struct stat istat;
	struct iblok *ip;
	const char *errfn;

	if (fn[0] == '-' && fn[1] == '\0') {
		ip = ib_alloc(0, Bflag);
		errfn = "stdin";
	} else {
		if ((ip = ib_open(fn, Bflag)) == NULL) {
			if (sflag == 0)
				fprintf(stderr, "%s: cannot open %s: %s\n",
						progname, fn, strerror(errno));
			errcnt |= 1;
			return;
		}
		errfn = fn;
	}
	if (fstat(ip->ib_fd, &istat) < 0) {
		if (sflag == 0)
			fprintf(stderr, "%s: cannot stat %s: %s\n", progname,
					errfn, strerror(errno));
		errcnt |= 1;
		if (ip->ib_fd)
			ib_close(ip);
		else
			ib_free(ip);
		return;
	}
	if ((ostat.st_mode&S_IFMT) != S_IFCHR &&
			(ostat.st_mode&S_IFMT) != S_IFBLK &&
			(ostat.st_mode&S_IFMT) != S_IFIFO &&
			istat.st_dev == ostat.st_dev &&
			istat.st_ino == ostat.st_ino) {
		if (sflag == 0)
			fprintf(stderr,
				"%s: input/output files '%s' identical\n",
				progname, errfn);
		errcnt |= 1;
		if (ip->ib_fd)
			ib_close(ip);
		else
			ib_free(ip);
		return;
	}
	if (vflag) {
		char	*cp;
		wint_t	wc;
		int	m, n;
		char	b;

		while ((cp = multibyte ? ib_getw(ip, &wc, &n) :
				(wc = ib_get(ip)) == (wint_t)EOF ? NULL :
					(b = (char)wc, n = 1, &b))
				!= NULL) {
			if (wc == '\n') {
				if (eflag)
					ob_put('$', op);
				if (ob_put(wc, op) == EOF)
					goto err;
			} else if (tflag == 0 && (wc == '\t' || wc == '\f')) {
				ob_put(wc, op);
			} else if (wc != WEOF && (wc < 040 || wc == 0177)) {
				ob_put('^', op);
				ob_put(wc ^ 0100, op);
			} else if (multibyte ? iswprint(wc) : isprint(wc)) {
				for (m = 0; m < n; m++)
					ob_put(cp[m], op);
			} else {
				for (m = 0; m < n; m++) {
					wc = cp[m] & 0377;
					if (wc & 0200) {
						ob_put('M', op);
						ob_put('-', op);
					}
					wc &= 0177;
					if (wc < 040 || wc == 0177) {
						ob_put('^', op);
						ob_put(wc ^ 0100, op);
					} else
						ob_put(wc, op);
				}
			}
		}
		if (ob_flush(op) != 0)
			goto err;
	} else {
		size_t sz;

#ifdef	__linux__
		if (!Bflag && istat.st_size > 0 && op->ob_bf == OB_FBF) {
			long long	sent;

			ob_flush(op);
			if ((sent = sfile(op->ob_fd, ip->ib_fd, istat.st_mode,
					istat.st_size)) == istat.st_size)
				goto closeit;
			if (sent < 0)
				goto err;
		}
#endif	/* __linux__ */
		while (ib_read(ip) != EOF) {
			ip->ib_cur--;
			sz = ip->ib_end - ip->ib_cur;
			if (ob_write(op, ip->ib_cur, sz) != sz)
				goto err;
		}
	}
	if (ip->ib_errno) {
		if (sflag == 0)
			fprintf(stderr, "%s: read error on file '%s': %s\n",
				progname, errfn, strerror(errno));
		errcnt |= 1;
	}
	goto closeit;
err:
closeit:
	if (ip->ib_fd > 0) {
		if (ib_close(ip) < 0) {
			if (sflag == 0)
				fprintf(stderr, "%s: close error on %s: %s\n",
					progname, errfn, strerror(errno));
			errcnt |= 1;
		}
	} else
		ib_free(ip);
}

int
main(int argc, char **argv)
{
	struct oblok	*op;
	int	i;

#ifdef	__GLIBC__
	putenv("POSIXLY_CORRECT=1");
#endif
	progname = basename(argv[0]);
	setlocale(LC_CTYPE, "");
	multibyte = MB_CUR_MAX > 1;
	while ((i = getopt(argc, argv, "suvetB:")) != EOF) {
		switch (i) {
		case 's':
			sflag = 1;
			break;
		case 'u':
			uflag = 1;
			break;
		case 'v':
			vflag = 1;
			break;
		case 'e':
			eflag = 1;
			break;
		case 't':
			tflag = 1;
			break;
		case 'B':
			Bflag = atol(optarg);
			break;
		default:
			usage();
		}
	}
	op = ob_alloc(1, uflag || Bflag&&!vflag ? OB_NBF : OB_EBF);
	if (fstat(op->ob_fd, &ostat) < 0) {
		if (sflag == 0)
			fprintf(stderr, "%s: Cannot stat stdout\n", progname);
		exit(1);
	}
	if (optind != argc) {
		while (optind < argc)
			copy(argv[optind++], op);
	} else
		copy("-", op);
	ob_free(op);
	if (close(1) < 0) {
		if (sflag == 0)
			fprintf(stderr, "%s: close error: %s\n",
				progname, strerror(errno));
		errcnt |= 4;
	}
	return errcnt;
}
