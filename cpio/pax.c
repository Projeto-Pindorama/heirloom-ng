/*
 * cpio - copy file archives in and out
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, April 2003.
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
#if defined (SU3)
static const char sccsid[] USED = "@(#)pax_su3.sl	1.26 (gritter) 6/26/05";
#else
static const char sccsid[] USED = "@(#)pax.sl	1.26 (gritter) 6/26/05";
#endif
/*	Sccsid @(#)pax.c	1.26 (gritter) 6/26/05	*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <fnmatch.h>
#include <dirent.h>
#include <regex.h>
#include <wchar.h>
#include <time.h>
#include <inttypes.h>

#include "iblok.h"
#include "cpio.h"

static char		**files;
static int		filec;
static struct iblok	*filinp;
static char		*path;
static size_t		pathsz;
static int		pax_Hflag;

static void		setpres(const char *);
static size_t		ofiles_pax(char **, size_t *);
static void		prtime_pax(time_t);
static void		parsesub(char *);

void
flags(int ac, char **av)
{
	const char	optstring[] = "rwab:cdf:HikKlLno:p:s:tuvx:X";
	int	i;
	int	illegal = 0;
	char	*x;

#if defined (SU3)
	pax = PAX_TYPE_PAX2001;
#else
	pax = PAX_TYPE_PAX1992;
#endif
	dflag = 1;
	uflag = 1;
	ofiles = ofiles_pax;
	prtime = prtime_pax;
	while ((i = getopt(ac, av, optstring)) != EOF) {
		switch (i) {
		case 'r':
			if (action && action != 'i')
				action = 'p';
			else
				action = 'i';
			break;
		case 'w':
			if (action && action != 'o')
				action = 'p';
			else
				action = 'o';
			break;
		case 'a':
			Aflag = 1;
			break;
		case 'b':
			blksiz = strtol(optarg, &x, 10);
			switch (*x) {
			case 'b':
				blksiz *= 512;
				break;
			case 'k':
				blksiz *= 1024;
				break;
			case 'm':
				blksiz *= 1048576;
				break;
			case 'w':
				blksiz *= 2;
				break;
			}
			if (blksiz <= 0)
				msg(4, -2,
					"Illegal size given for -b option.\n");
			Cflag = 1;
			break;
		case 'c':
			fflag = 1;
			break;
		case 'd':
			pax_dflag = 1;
			break;
		case 'f':
			Oflag = Iflag = optarg;
			break;
		case 'H':
			pax_Hflag = 1;
			break;
		case 'i':
			rflag = 1;
			break;
		case 'k':
			pax_kflag = 1;
			break;
		case 'K':
			kflag = 1;
			break;
		case 'l':
			lflag = 1;
			break;
		case 'L':
			Lflag = 1;
			break;
		case 'n':
			pax_nflag = 1;
			break;
		case 'o':
			pax_options(optarg, 1);
			break;
		case 'p':
			setpres(optarg);
			break;
		case 's':
			pax_sflag = 1;
			parsesub(optarg);
			break;
		case 't':
			aflag = 1;
			break;
		case 'u':
			uflag = 0;
			pax_uflag = 1;
			break;
		case 'v':
			vflag = 1;
			break;
		case 'x':
			if (strcmp(optarg, "cpio") == 0)
				fmttype = FMT_ODC;
			else {
				if (setfmt(optarg) < 0)
					illegal = 1;
			}
			break;
		case 'X':
			pax_Xflag = 1;
			break;
		default:
			illegal = 1;
		}
	}
	switch (action) {
	case 0:
		if (rflag || pax_kflag || pax_uflag || pax_preserve)
			illegal = 1;
		action = 'i';
		tflag = 1;
		setvbuf(stdout, NULL, _IOLBF, 0);
		/*FALLTHRU*/
	case 'i':
		if (aflag || pax_Xflag || lflag)
			illegal = 1;
		for (i = optind; i < ac; i++) {
			addg(av[i], 0);
			if (pax_dflag == 0) {
				char	*da;
				int	j;

				da = smalloc(strlen(av[i]) + 2);
				for (j = 0; av[i][j]; j++)
					da[j] = av[i][j];
				da[j++] = '/';
				da[j++] = '*';
				da[j] = 0;
				addg(da, 1);
				free(da);
			}
		}
		break;
	case 'o':
		if (fflag || pax_kflag || pax_nflag || kflag)
			illegal = 1;
		if (Aflag && Oflag == NULL) {
			msg(3, 0, "-a requires the -f option\n");
			illegal = 1;
		}
		if (optind != ac) {
			files = &av[optind];
			filec = ac - optind;
		} else
			filinp = ib_alloc(0, 0);
		if (pax_uflag)
			Aflag = 1;
		if (Aflag == 0 && fmttype == FMT_NONE)
			fmttype = FMT_ODC;
		break;
	case 'p':
		if (fflag || blksiz || Oflag || Iflag || fmttype != FMT_NONE ||
				kflag)
			illegal = 1;
		if (optind == ac)
			illegal = 1;
		else if (optind+1 != ac) {
			files = &av[optind];
			filec = ac - optind - 1;
			optind = ac - 1;
		} else
			filinp = ib_alloc(0, 0);
		break;
	}
	if (illegal)
		usage();
}

void
usage(void)
{
	fprintf(stderr, "USAGE:\n\
\t%s [-cdnvK] [-b size] [-f file] [-s replstr] [-x hdr] [patterns]\n\
\t%s -r[cdiknuvK] [-b size] [-f file] [-p priv] [-s replstr] [-x hdr] [patterns]\n\
\t%s -w[adituvLX] [-b size] [-f file] [-s replstr] [-x hdr] [files]\n\
\t%s -rw[diklntuvLX] [-p priv] [-s replstr] [files] directory\n",
		progname, progname, progname, progname);
	exit(1);
}

static void
setpres(const char *s)
{
	s--;
	while (*++s) {
		pax_preserve &= ~PAX_P_EVERY;
		switch (*s) {
		case 'a':
			pax_preserve |= PAX_P_ATIME;
			break;
		case 'e':
			pax_preserve |= PAX_P_EVERY;
			break;
		case 'm':
			pax_preserve |= PAX_P_MTIME;
			break;
		case 'o':
			pax_preserve |= PAX_P_OWNER;
			break;
		case 'p':
			pax_preserve |= PAX_P_MODE;
			break;
		default:
			msg(2, 0, "ignoring unknown option \"-p%c\"\n",
					*s&0377);
		}
	}
	if (pax_preserve & PAX_P_EVERY)
		pax_preserve |= PAX_P_OWNER|PAX_P_MODE;
}

int
gmatch(const char *s, const char *p)
{
	int	val;
#ifdef	__GLIBC__
	/* avoid glibc's broken [^...] */
	extern char	**environ;
	char	**savenv = environ;
	char	*newenv[] = { "POSIXLY_CORRECT=", NULL };
	environ = newenv;
#endif	/* __GLIBC__ */
	val = fnmatch(p, s, 0) == 0;
#ifdef	__GLIBC__
	environ = savenv;
#endif	/* __GLIBC__ */
	return val;
}

static const char *
nextfile(void)
{
	char	*line = NULL;
	size_t	linsiz = 0, linlen;

	if (filinp) {
		pax_Hflag = 0;
		if ((linlen=ib_getlin(filinp, &line, &linsiz, srealloc)) == 0) {
			filinp = NULL;
			return NULL;
		}
		if (line[linlen-1] == '\n')
			line[--linlen] = '\0';
		return line;
	} else if (filec > 0) {
		filec--;
		return *files++;
	} else
		return NULL;
}

static size_t
catpath(size_t pend, const char *base)
{
	size_t	blen = strlen(base);

	if (pend + blen + 2 >= pathsz)
		path = srealloc(path, pathsz = pend + blen + 16);
	if (pend == 0 || path[pend-1] != '/')
		path[pend++] = '/';
	strcpy(&path[pend], base);
	return pend + blen;
}

/*
 * Descend the directory hierarchies given using stdin or arguments
 * and return file names one per one.
 */
static size_t
ofiles_pax(char **name, size_t *namsiz)
{
	static DIR	**dt;
	static int	dti, dts;
	static int	*pend;
	static dev_t	*curdev;
	static ino_t	*curino;
	struct stat	st;
	struct dirent	*dp;
	const char	*nf;
	int	i;

	if (dt == NULL) {
		dt = scalloc(dts = 1, sizeof *dt);
		pend = scalloc(dts, sizeof *pend);
		curdev = scalloc(dts, sizeof *curdev);
		curino = scalloc(dts, sizeof *curino);
	}
	for (;;) {
		if (dti >= 0 && dt[dti] != NULL) {
			if ((dp = readdir(dt[dti])) != NULL) {
				if (dp->d_name[0] == '.' &&
						(dp->d_name[1] == '\0' ||
						 dp->d_name[1] == '.' &&
						 dp->d_name[2] == '\0'))
					continue;
				if (dti+1 <= dts) {
					dt = srealloc(dt, sizeof *dt * ++dts);
					pend = srealloc(pend, sizeof *pend*dts);
					curdev = srealloc(curdev, sizeof *curdev
							* dts);
					curino = srealloc(curino, sizeof *curino
							* dts);
				}
				pend[dti+1] = catpath(pend[dti], dp->d_name);
				if (pax_Hflag)
					Lflag = dti < 0;
				if ((Lflag ? stat : lstat)(path, &st) < 0) {
					emsg(2, "Error with %s of \"%s\"",
							lflag? "stat" : "lstat",
							path);
					errcnt++;
				} else if ((st.st_mode&S_IFMT) == S_IFDIR &&
						(pax_Xflag == 0 ||
						 curdev[0] == st.st_dev)) {
					if (Lflag) {
						for (i = 0; i <= dti; i++)
							if (st.st_dev ==
								curdev[i] &&
								st.st_ino ==
								curino[i]) {
							    if (pax ==
							      PAX_TYPE_PAX2001)
							     msg(4, 1,
								"Symbolic link "
								"loop at "
								"\"%s\"\n",
								path);
							    break;
							}
						if (i <= dti)
							break;
					}
					if ((dt[dti+1]=opendir(path)) == NULL) {
						emsg(2, "Cannot open directory "
								"\"%s\"", path);
						errcnt++;
					} else {
						dti++;
						curdev[dti] = st.st_dev;
						curino[dti] = st.st_ino;
						continue;
					}
				} else
					break;
			} else {
				path[pend[dti]] = '\0';
				closedir(dt[dti]);
				dt[dti--] = NULL;
				if (pax_Hflag)
					Lflag = dti < 0;
				break;
			}
		} else {
			if (pax_Hflag)
				Lflag = 1;
			while ((nf = nextfile()) != NULL &&
					(Lflag ? stat : lstat)(nf, &st) < 0) {
				emsg(2, "Error with stat of \"%s\"", nf);
				errcnt++;
			}
			if (nf == NULL)
				return 0;
			dti = 0;
			if (path)
				free(path);
			pend[dti] = strlen(nf);
			strcpy(path = smalloc(pathsz = pend[dti]+1), nf);
			if (pax_dflag || (st.st_mode&S_IFMT) != S_IFDIR) {
				dti = -1;
				break;
			}
			curdev[dti] = st.st_dev;
			curino[dti] = st.st_ino;
			if ((dt[dti] = opendir(path)) == NULL) {
				emsg(2, "Cannot open directory \"%s\"", path);
				errcnt++;
			}
		}
	}
	if (*name == NULL || *namsiz < pathsz) {
		free(*name);
		*name = smalloc(*namsiz=pathsz);
	}
	strcpy(*name, path);
	return pend[dti+1];
}

struct	pax_had {
	struct pax_had	*p_next;
	const char	*p_name;
	time_t		p_mtime;
};

static int	pprime = 7919;

static int
phash(const char *s)
{
	uint32_t	h = 0, g;

	s--;
	while (*++s) {
		h = (h << 4) + (*s & 0377);
		if (g = h & 0xf0000000) {
			h = h ^ (g >> 24);
			h = h ^ g;
		}
	}
	return h % pprime;
}

static int
plook(const char *name, struct pax_had **pp)
{
	static struct pax_had	**pt;
	uint32_t	h, had;

	if (pt == NULL)
		pt = scalloc(pprime, sizeof *pt);
	(*pp) = pt[h = phash(name)];
	while (*pp != NULL) {
		if (strcmp((*pp)->p_name, name) == 0)
			break;
		*pp = (*pp)->p_next;
	}
	had = *pp != NULL;
	if (*pp == NULL) {
		*pp = scalloc(1, sizeof **pp);
		(*pp)->p_name = sstrdup(name);
		(*pp)->p_next = pt[h];
		pt[h] = *pp;
	}
	return had;
}

int
pax_track(const char *name, time_t mtime)
{
	struct pax_had	*pp;
	struct stat	st;

	if (pax_uflag == 0 && (pax_nflag == 0 || patterns))
		return 1;
	if (action == 'i' && pax_uflag) {
		if (lstat(name, &st) == 0 && mtime < st.st_mtime)
			return 0;
	}
	if (action != 'i' || pax_nflag) {
		if (plook(name, &pp) != 0) {
			if (action != 'i' && pax_uflag == 0)
				return 0;
			if (mtime > pp->p_mtime) {
				pp->p_mtime = mtime;
				return 1;
			}
			return 0;
		} else
			pp->p_mtime = mtime;
	}
	return 1;
}

static void
prtime_pax(time_t t)
{
	char	b[30];
	time_t	now;

	time(&now);
	if (t > now || t < now - (6*30*86400))
		strftime(b, sizeof b, "%b %e  %Y", localtime(&t));
	else
		strftime(b, sizeof b, "%b %e %H:%M", localtime(&t));
	printf(" %s ", b);
}

struct replacement {
	regex_t			r_re;
	const char		*r_rhs;
	int			r_nbra;
	enum {
		REPL_0	= 0,
		REPL_G	= 1,
		REPL_P	= 2
	}			r_flags;
} *rep;

#define	NBRA	9
static int	ren, res;
static int	mb_cur_max;

static wchar_t
nextc(char **sc, int *np)
{
	char	*p = *sc;
	wchar_t	wcbuf;
	int	len;

	if (**sc == '\0') {
		*np = 0;
		return 0;
	}
	if (mb_cur_max == 1 || (**sc&0200) == 0) {
		*np = 1;
		return *(*sc)++ & 0377;
	}
	if ((len = mbtowc(&wcbuf, p, mb_cur_max)) < 0)
		msg(3, -2, "Invalid multibyte character for \"-s\" option\n");
	*np = len;
	*sc += len;
	return wcbuf;
}

static void
parsesub(char *s)
{
	int	len;
	char	*ps = NULL;
	wchar_t	seof = nextc(&s, &len);
	wint_t	c, d;
	int	nbra = 0;
	int	reflags;

	if (seof == 0)
		goto unt;
	mb_cur_max = MB_CUR_MAX;
	ps = s;
	do {
		if ((c = nextc(&s, &len)) == seof)
			break;
		if (c == '\\') {
			if ((c = nextc(&s, &len)) == '(')
				nbra++;
			continue;
		} else if (c == '[') {
			d = WEOF;
			do {
				if ((c = nextc(&s, &len)) == '\0')
					continue;
				if (d == '[' && (c == ':' || c == '.' ||
							c == '=')) {
					d = c;
					do {
						if ((c=nextc(&s, &len)) == '\0')
							continue;
					} while (c != d || *s != ']');
					nextc(&s, &len);
					c = WEOF; /* reset d and continue */
				}
				d = c;
			} while (c != ']');
		}
	} while (*s != '\0');
	if (c != seof)
	unt:	msg(3, -2, "Unterminated argument for \"-s\" option.\n");
	s[-len] = '\0';
	if (ren <= res)
		rep = srealloc(rep, ++res * sizeof *rep);
	reflags = REG_ANGLES;
	if (pax >= PAX_TYPE_PAX2001)
		reflags |= REG_AVOIDNULL;
	if (regcomp(&rep[ren].r_re, ps, reflags) != 0)
		msg(3, -2, "Regular expression error in \"-s\" option\n");
	rep[ren].r_rhs = s;
	rep[ren].r_nbra = nbra;
	while ((c = nextc(&s, &len)) != 0) {
		if (c == '\\')
			c = nextc(&s, &len);
		else if (c == seof)
			break;
	}
	rep[ren].r_flags = 0;
	if (c == seof) {
		s[-len] = '\0';
		while ((c = nextc(&s, &len)) != '\0') {
			switch (c) {
			case 'g':
				rep[ren].r_flags |= REPL_G;
				break;
			case 'p':
				rep[ren].r_flags |= REPL_P;
				break;
			default:
				msg(2, 0, "Ignoring unknown -s flag \"%c\"\n",
						c);
			}
		}
	}
	ren++;
}

#define	put(c)	((new = innew+1>=newsize ? srealloc(new, newsize+=32) : new), \
			new[innew++] = (c))

int
pax_sname(char **oldp, size_t *olds)
{
	char	*new = NULL;
	size_t	newsize = 0;
	regmatch_t	bralist[NBRA+1];
	int	c, i, k, l, y, z;
	int	innew = 0, ef = 0;
	char	*inp = *oldp;

	for (z = 0; z < ren; z++) {
	in:	if (regexec(&rep[z].r_re, inp, NBRA+1, bralist, ef) != 0) {
			if (ef == 0)
				continue;
			goto out;
		}
		for (i = 0; i < bralist[0].rm_so; i++)
			put(inp[i]);
		k = 0;
		while (c = rep[z].r_rhs[k++] & 0377) {
			y = -1;
			if (c == '&')
				y = 0;
			else if (c == '\\') {
				c = rep[z].r_rhs[k++] & 0377;
				if (c >= '1' && c < rep[z].r_nbra+'1')
					y = c - '0';
			}
			if (y >= 0)
				for (l = bralist[y].rm_so; l < bralist[y].rm_eo;
						l++)
					put(inp[l]);
			else
				put(c);
		}
		k = innew;
		for (i = bralist[0].rm_eo; inp[i]; i++)
			put(inp[i]);
		put('\0');
		if (rep[z].r_flags & REPL_G) {
			ef = REG_NOTBOL;
			inp = &inp[bralist[0].rm_eo];
			innew = k;
			if (bralist[0].rm_so == bralist[0].rm_eo) {
				if (inp[0] && (nextc(&inp, &l), inp[0]))
					innew++;
				else
					goto out;
			}
			goto in;
		}
	out:	if (rep[z].r_flags & REPL_P)
			fprintf(stderr, "%s >> %s\n", *oldp, new);
		free(*oldp);
		*oldp = new;
		*olds = newsize;
		return *new != '\0';
	}
	return 1;
}

void
pax_onexit(void)
{
	struct glist	*gp;

	for (gp = patterns; gp; gp = gp->g_nxt) {
		if (gp->g_art)
			continue;
		if (gp->g_gotcha == 0 && (gp->g_nxt == NULL ||
					gp->g_nxt->g_art == 0 ||
					gp->g_gotcha == 0)) {
			msg(3, 0, "Pattern not matched: \"%s\"\n", gp->g_pat);
			errcnt++;
		}
	}
}
