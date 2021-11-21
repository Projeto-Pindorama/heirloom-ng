/*	from Unix 32V /usr/src/cmd/calendar/calendar.c	*/
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
