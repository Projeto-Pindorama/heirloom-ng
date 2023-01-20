/*	from Unix 32V /usr/src/cmd/calendar/calendar.c	*/
/*
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * SPDX-Licence-Identifier: Caldera
 */

#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
static const char sccsid[] USED = "@(#)/usr/5lib/calprog.sl	1.6 (gritter) 5/29/05";

/* /usr/lib/calendar produces an egrep -f file
   that will select today's and tomorrow's
   calendar entries, with special weekend provisions

   used by calendar command
*/
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <langinfo.h>
#include <wctype.h>
#include <wchar.h>

#define DAY (3600*24L)

char *month[12];

void
fillone(char *abmon, char **tofill)
{
	wchar_t w[8];
	size_t sz;

	*tofill = malloc(MB_CUR_MAX * 7);
	mbstowcs(w, abmon, 7);
	(*tofill)[0] = '[';
	sz = 1;
	sz += wctomb(&(*tofill)[sz], towupper(w[0]));
	sz += wctomb(&(*tofill)[sz], towlower(w[0]));
	(*tofill)[sz++] = ']';
	sz += wctomb(&(*tofill)[sz], w[1]);
	sz += wctomb(&(*tofill)[sz], w[2]);
	(*tofill)[sz] = '\0';
}


#define	MONTH(x)	fillone(nl_langinfo(ABMON_##x), &month[(x - 1)])

void
init(void)
{
	MONTH(1);
	MONTH(2);
	MONTH(3);
	MONTH(4);
	MONTH(5);
	MONTH(6);
	MONTH(7);
	MONTH(8);
	MONTH(9);
	MONTH(10);
	MONTH(11);
	MONTH(12);
}

void
tprint(time_t t)
{
	struct tm *tm;
	char *d_fmt = nl_langinfo(D_FMT);

	tm = localtime(&t);
	/*
	 * XXX This is a 90% solution only.
	 */
	if (strstr(d_fmt, "%m") < strstr(d_fmt, "%d"))
		printf(
	"(^|[ (,;])((%s[^ ]* *|0%d[-.,/ ]|\\*[-.,/ ])0*%d)([^0123456789]|$)\n",
			month[tm->tm_mon], tm->tm_mon + 1, tm->tm_mday);
	else
		printf(
		"(^|[ (,;])(0*%d[^0123456789]*(%s[^ ]* *|%d[^0123456789]))\n",
			tm->tm_mday, month[tm->tm_mon], tm->tm_mon + 1);
}

int
main(void)
{
	time_t t;

	setlocale(LC_CTYPE, "");
	setlocale(LC_TIME, "");
	init();
	time(&t);
	tprint(t);
	switch(localtime(&t)->tm_wday) {
	case 5:
		t += DAY;
		tprint(t);
	case 6:
		t += DAY;
		tprint(t);
	default:
		t += DAY;
		tprint(t);
	}
	return(0);
}
