/*	from Unix 32V /usr/src/cmd/cal.c	*/
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

#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char sccsid[] USED = "@(#)cal.sl	1.12 (gritter) 5/29/05";

#include	<unistd.h>
#include	<time.h>
#include	<stdio.h>
#include	<libgen.h>
#include	<stdlib.h>
#include	<locale.h>
#include	<langinfo.h>
#include	<string.h>
#include	<wchar.h>
#include	<limits.h>

static wchar_t	dayw[] = {
	' ', 'S', ' ', ' ', 'M', ' ', 'T', 'u', ' ', ' ', 
	'W', ' ', 'T', 'h', ' ', ' ', 'F', ' ', ' ', 'S', 0
};
static char	*smon[]= {
	"January", "February", "March", "April",
	"May", "June", "July", "August",
	"September", "October", "November", "December"
};
static char	*amon[] = {
	"Jan", "Feb", "Mar", "Apr",
	"May", "Jun", "Jul", "Aug",
	"Sep", "Oct", "Nov", "Dec"
};
static char	string[432];
static char	*progname;

static void	usage(void);
static void	bad(void);
static void	dolocale(void);
static int	number(const char *str);
static void	pstr(char *str, int n);
static void	cal(int m, int y, char *p, int w);
static int	jan1(int yr);
static void	wput(const wchar_t *);

int
main(int argc, char **argv)
{
	register int y = 1, i, j;
	int m = 1;

	progname = basename(argv[0]);
	dolocale();
	if (argc > 1 && argv[1][0] == '-' && argv[1][1] == '-'
			&& argv[1][2] == '\0')
		argv++, argc--;
	if (argc == 2)
		goto xlong;
	else if (argc == 1) {
		time_t t;
		struct tm *tp;

		time(&t);
		tp = localtime(&t);
		m = tp->tm_mon + 1;
		y = tp->tm_year + 1900;
	} else if (argc == 3) {
		/*
 		 *	print out just month
 		 */
		m = number(argv[optind]);
		if(m<1 || m>12)
			bad();
		y = number(argv[optind + 1]);
		if(y<1 || y>9999)
			bad();
	} else
		usage();
	printf("   %s %u\n", smon[m-1], y);
	wput(dayw);
	putchar('\n');
	cal(m, y, string, 24);
	for(i=0; i<6*24; i+=24)
		pstr(string+i, 24);
	exit(0);

/*
 *	print out complete year
 */

xlong:
	y = number(argv[optind]);
	if(y<1 || y>9999)
		bad();
	printf("\n\n\n");
	printf("				%u\n", y);
	printf("\n");
	for(i=0; i<12; i+=3) {
		for(j=0; j<6*72; j++)
			string[j] = '\0';
		printf("	 %.3s", amon[i]);
		printf("			%.3s", amon[i+1]);
		printf("		       %.3s\n", amon[i+2]);
		wput(dayw);
		printf("   ");
		wput(dayw);
		printf("   ");
		wput(dayw);
		putchar('\n');
		cal(i+1, y, string, 72);
		cal(i+2, y, string+23, 72);
		cal(i+3, y, string+46, 72);
		for(j=0; j<6*72; j+=72)
			pstr(string+j, 72);
	}
	printf("\n\n\n");
	return 0;
}

static int
number(const char *str)
{
	register int n, c;
	register const char *s;

	n = 0;
	s = str;
	while((c = *s++) != '\0') {
		if(c<'0' || c>'9')
			return(0);
		n = n*10 + c-'0';
	}
	return(n);
}

static void
pstr(char *str, int n)
{
	register int i;
	register char *s;

	s = str;
	i = n;
	while(i--)
		if(*s++ == '\0')
			s[-1] = ' ';
	i = n+1;
	while(i--)
		if(*--s != ' ')
			break;
	s[1] = '\0';
	printf("%s\n", str);
}

char	mon[] = {
	0,
	31, 29, 31, 30,
	31, 30, 31, 31,
	30, 31, 30, 31,
};

static void
cal(int m, int y, char *p, int w)
{
	register int d, i;
	register char *s;

	s = p;
	d = jan1(y);
	mon[2] = 29;
	mon[9] = 30;

	switch((jan1(y+1)+7-d)%7) {

	/*
	 *	non-leap year
	 */
	case 1:
		mon[2] = 28;
		break;

	/*
	 *	1752
	 */
	default:
		mon[9] = 19;
		break;

	/*
	 *	leap year
	 */
	case 2:
		;
	}
	for(i=1; i<m; i++)
		d += mon[i];
	d %= 7;
	s += 3*d;
	for(i=1; i<=mon[m]; i++) {
		if(i==3 && mon[m]==19) {
			i += 11;
			mon[m] += 11;
		}
		if(i > 9)
			*s = i/10+'0';
		s++;
		*s++ = i%10+'0';
		s++;
		if(++d == 7) {
			d = 0;
			s = p+w;
			p = s;
		}
	}
}

/*
 *	return day of the week
 *	of jan 1 of given year
 */

static int
jan1(int yr)
{
	register int y, d;

/*
 *	normal gregorian calendar
 *	one extra day per four years
 */

	y = yr;
	d = 4+y+(y+3)/4;

/*
 *	julian calendar
 *	regular gregorian
 *	less three days per 400
 */

	if(y > 1800) {
		d -= (y-1701)/100;
		d += (y-1601)/400;
	}

/*
 *	great calendar changeover instant
 */

	if(y > 1752)
		d += 3;

	return(d%7);
}

static void
usage(void)
{
	fprintf(stderr, "usage: %s [ [month] year ]\n", progname);
	exit(2);
}

static void
bad(void)
{
	fprintf(stderr, "Bad argument\n");
	exit(1);
}

#define	COPY_DAY(d)		cp = nl_langinfo(DAY_##d); \
				cp += mbtowc(wq++, cp, MB_CUR_MAX); \
				cp += mbtowc(wq++, cp, MB_CUR_MAX); \
				wq++

#define	COPY_MONTH(m)		smon[m - 1] = strdup(nl_langinfo(MON_##m))

#define	COPY_ABMON(m)		amon[m - 1] = strdup(nl_langinfo(ABMON_##m))

static void
dolocale(void)
{
	char *cp;
	wchar_t *wq;

#ifndef	__dietlibc__
	setlocale(LC_CTYPE, "");
	cp = setlocale(LC_TIME, "");
	if (cp && strcmp(cp, "C") && strcmp(cp, "POSIX") && strcmp(cp, "en") &&
			strncmp(cp, "en_", 3)) {
#else	/* __dietlibc__ */
	if (1) {
#endif	/* __dietlibc__ */
		wq = dayw;
		COPY_DAY(1);
		COPY_DAY(2);
		COPY_DAY(3);
		COPY_DAY(4);
		COPY_DAY(5);
		COPY_DAY(6);
		COPY_DAY(7);
		COPY_MONTH(1);
		COPY_MONTH(2);
		COPY_MONTH(3);
		COPY_MONTH(4);
		COPY_MONTH(5);
		COPY_MONTH(6);
		COPY_MONTH(7);
		COPY_MONTH(8);
		COPY_MONTH(9);
		COPY_MONTH(10);
		COPY_MONTH(11);
		COPY_MONTH(12);
		COPY_ABMON(1);
		COPY_ABMON(2);
		COPY_ABMON(3);
		COPY_ABMON(4);
		COPY_ABMON(5);
		COPY_ABMON(6);
		COPY_ABMON(7);
		COPY_ABMON(8);
		COPY_ABMON(9);
		COPY_ABMON(10);
		COPY_ABMON(11);
		COPY_ABMON(12);
	}
}

static void
wput(const wchar_t *s)
{
	char	mb[MB_LEN_MAX];
	int	i, n;

	s--;
	while (*++s) {
		n = wctomb(mb, *s);
		for (i = 0; i < n; i++)
			putchar(mb[i] & 0377);
	}
}
