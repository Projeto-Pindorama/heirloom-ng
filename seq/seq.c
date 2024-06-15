/*
 * seq.c - print a sequence of numbers
 */
/* 
 * Copyright (C) 2023: Luiz Antônio Rangel (takusuman)
 *
 * SPDX-Licence-Identifier: Zlib
 */

#include <ctype.h>
#include <pfmt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* That's is for custom format at printf(3) via -p and -w. */
#pragma clang diagnostic ignored "-Wformat-nonliteral"
/* False positive for char *argv[]. */
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
/* main() exit()s, does not return(). */
#pragma clang diagnostic ignored "-Wmain-return-type"

static char *progname;
/* 
 * Make flags public since it will
 * be used by buildfmt().
 */
static int fPicture = 0,
	   fWadding = 0;
static char *picstr = "";
static float start = 0,
	     stop = 0,
	     step = 0;

void main(int argc, char *argv[]);
char *buildfmt(void);
char *getlgstr(void);
int afterdecsep(char *s);
void usage(void);

void main(int argc, char *argv[]) {
	progname = argv[0];
	extern int optind;
	int option = 0;
	register float count = 0;
	char *format = "",
		*separator = "";
	
/*	while ((!isdigit(argv[optind][1]) && (argc > optind)) */
	while (!isdigit(argv[optind][1])
		&& (option = getopt(argc, argv, ":p:s:w")) != -1 ) {
		switch (option) {
			case 'p':
				fPicture = 1;
				picstr = optarg;
				break;
			case 's':
				separator = optarg;
				break;
			case 'w':
				fWadding = 1;
				break;
			default:
				usage();
		}
	}
	argc -= optind;
	argv += optind;
	
	if ( argc < 1 ) {
		usage();
	}

	/* 
	 * If argc equals 1, stop will be defined as the first command line argument
	 * and start and step will be defaulted to 1.
	 * If argc is 2, stop will be defined as the second command line argument and
	 * start as the first.
	 * If argc is 3, stop will be defined as the third, start as the first and
	 * step as the second.
	 */
	stop = ( argc == 1 )
		? atof(argv[0])
		: ( argc == 3 )
		? atof(argv[2])
		: atof(argv[1]);
	start = ( argc == 1 )
		? 1
		: atof(argv[0]);
	step = ( argc < 3 )
		? 1
		: atof(argv[1]);
	
	if (step == 0) {
		pfmt(stderr, MM_ERROR,
			"%s: increment can not be zero.\n",
			progname);
		exit(1);
	}

	format = buildfmt();

	/* If there's no separator set, defaults to a line break (\n). */
	separator = (! *separator)
			? "\n"
			: separator;

	for (count = start; count <= stop; count += step) {
		/* 
		 * If the count has come to the end or if the next sum is
		 * larger than stop, default separator back to '\n'.
		 */
		separator = (count == stop || (count + step) > stop)
				? "\n" : separator;
		printf(format, count, separator);
	}
	free(format);

	exit(0);
}

char *buildfmt(void) {
	char *picture = "",
	     *fmtbuf = ""; 
	
	if ((fmtbuf = calloc(32, sizeof(char *))) == NULL) {
		pfmt(stderr, MM_ERROR, "%s: could not allocate an "
			"array of %d elements, each one "
			"being %lu bytes large.\n",
			progname, 32, (sizeof(char *)));
		exit(1);
	}

	/* Default. */
	if (!fPicture && !fWadding) {
		snprintf(fmtbuf, sizeof(fmtbuf), "%s", "%g%s");
		return fmtbuf;
	} 
	
	if (fPicture || fWadding) { 
		long int precision = 0;
		unsigned long int natural = 0;
			      
		char strnum[32] = "",
			/* 
			 * Unlike the default,
			 * creating a buffer will
			 * be needed for avoiding
			 * truncating 'fmtbuf'.
			 */
		     buf[32] = "";

		/* 
		 * Get how many digits after the
		 * decimal separator the picture
		 * have.
		 * If wadding is enabled alone, we
		 * shall check for the longest
		 * number on the buffer, if not,
		 * just the picture number.
		 */
		picture = (!fPicture && fWadding)
			? getlgstr()
			: picstr;
		precision = afterdecsep(picture);

		if (precision == -1) {
			pfmt(stderr, MM_ERROR,
				"%s: picture '%s' is not a number.\n",
				progname, picture);
			exit(1);
		}

		/* 
		 * Basically the procedement
		 * that afterdecsep() does,
		 * but as a one-liner.
		 */
		if (fWadding) {
			snprintf(strnum, sizeof(strnum), "%.0f",
								(start < 0)
								? start
								: stop);
		} else {
			snprintf(strnum, sizeof(strnum), "%.0f", picture);
		}
		natural = strlen(strnum);

		/* 
		 * free() only if picture comes
		 * from getlgstr(), since it had
		 * been allocated in memory, in
		 * contrast to picstr which is a
		 * vulgar string.
		 */
		if (!fPicture && fWadding) {
			free(picture);
		}
	
		/* 
		 * If there are decimal values, add it to
		 * "natural" plus one, since "natural" in
		 * printf(3) context means the total
		 * number of characters that will be
		 * printed.
		 */	
		if (precision > 0) {
			natural += (unsigned long int)precision;
			natural += 1;
		}

		/* Write to 'buf', then copy to fmtbuf. */
		snprintf(buf, sizeof(buf), "%%%d%ld.%ldf%%s",
					0, natural, precision);
		fmtbuf = strdup(buf);
		
		return fmtbuf;
	}

	return NULL;
}

char *getlgstr(void) {
	char strflt[32] = "",
	     *lgstnum = "";

	if ((lgstnum = calloc(sizeof(strflt), sizeof(char *))) == NULL) {
		pfmt(stderr, MM_ERROR, "%s: could not allocate an "
			"array of %d elements, each one "
			"being %lu bytes large.\n",
			progname, sizeof(strflt), (sizeof(char *)));
		exit(1);
	}

	/* 
	 * This method, although not being precise
	 * as the last, will get the most irregular
	 * number in the count per subtracting 'step'
	 * from the first number ('start'), which means
	 * getting the most of decimal numbers.
	 * It is better to subtract from the first
	 * number instead of the last because of numbers
	 * big enough to be converted by
	 * 'printf("%g", ...)' into scientific notation,
	 * which can not be parsed by afterdecsep().
	 */
	snprintf(strflt, sizeof(strflt), "%g", (start - step));
	lgstnum = strdup(strflt);
		
	return lgstnum;
}

int afterdecsep(char *s) {
	register int c = 0;
	unsigned long fraclen = 0;
	char *fracpart = "";

	if (isalpha(s[0])) {
		/* Panic. */
		return -1;
	}

	/* 
	 * If there's not a decimal separator at *s,
	 * it's an invalid picture, but seq(1) will
	 * be not complaining about it.
	 */
	if (strchr(s, '.') != NULL) {
		for (c = 0; s[c]; c++) {
			switch (s[c]) {
				case '.':
					fracpart = &s[(c + 1)];
					s[c] = '\0';
					break;
				default:
					continue;
			}
			break;
		}
		fraclen = strlen(fracpart);
	}

	return (int)fraclen;
}

void usage(void) {
	pfmt(stderr, MM_NOSTD,
		"usage: %s: [-w] [-ppicture] [-sseparator] [start [step]] stop\n",
		progname);
	exit(1);
}
