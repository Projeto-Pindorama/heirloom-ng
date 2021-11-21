/*
 * getopt() - command option parsing
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, March 2002.
 */

/*	Sccsid @(#)getopt.c	1.5 (gritter) 4/2/07	*/

#include	<sys/types.h>
#include	<alloca.h>
#include	<string.h>
#include	"msgselect.h"

/*
 * One should not think that re-implementing this is necessary, but
 *
 * - Some libcs print weird messages.
 *
 * - GNU libc getopt() is totally brain-damaged, as it requires special
 *   care _not_ to reorder parameters and can't be told to work correctly
 *   with ':' as first optstring character at all.
 */

char	*optarg = 0;
int	optind = 1;
int	opterr = 1;
int	optopt = 0;
extern char	*pfmt_label__;

static void
error(const char *s, int c)
{
	/*
	 * Avoid including <unistd.h>, in case its getopt() declaration
	 * conflicts.
	 */
	extern ssize_t	write(int, const void *, size_t);
	const char	*msg = 0;
	char	*buf, *bp;

	if (pfmt_label__)
		s = pfmt_label__;
	switch (c) {
	case '?':
		msg = ": " msgselect("I","i") "llegal option -- ";
		break;
	case ':':
		msg = ": " msgselect("O","o") "ption requires an argument -- ";
		break;
	}
	bp = buf = alloca(strlen(s) + strlen(msg) + 2);
	while (*s)
		*bp++ = *s++;
	while (*msg)
		*bp++ = *msg++;
	*bp++ = optopt;
	*bp++ = '\n';
	write(2, buf, bp - buf);
}

int
getopt(int argc, char *const argv[], const char *optstring)
{
	int	colon;
	static const char	*lastp;
	const char	*curp;

	if (optstring[0] == ':') {
		colon = 1;
		optstring++;
	} else
		colon = 0;
	if (lastp) {
		curp = lastp;
		lastp = 0;
	} else {
		if (optind >= argc || argv[optind] == 0 ||
				argv[optind][0] != '-' ||
				argv[optind][1] == '\0')
			return -1;
		if (argv[optind][1] == '-' && argv[optind][2] == '\0') {
			optind++;
			return -1;
		}
		curp = &argv[optind][1];
	}
	optopt = curp[0] & 0377;
	while (optstring[0]) {
		if (optstring[0] == ':') {
			optstring++;
			continue;
		}
		if ((optstring[0] & 0377) == optopt) {
			if (optstring[1] == ':') {
				if (curp[1] != '\0') {
					optarg = (char *)&curp[1];
					optind++;
				} else {
					if ((optind += 2) > argc) {
						if (!colon && opterr)
							error(argv[0], ':');
						return colon ? ':' : '?';
					}
					optarg = argv[optind - 1];
				}
			} else {
				if (curp[1] != '\0')
					lastp = &curp[1];
				else
					optind++;
				optarg = 0;
			}
			return optopt;
		}
		optstring++;
	}
	if (!colon && opterr)
		error(argv[0], '?');
	if (curp[1] != '\0')
		lastp = &curp[1];
	else
		optind++;
	optarg = 0;
	return '?';
}
