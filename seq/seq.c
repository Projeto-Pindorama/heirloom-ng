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
char *buildfmt();
char *getlgstr();
int afterdecsep(char *s);
void usage(void);

void main(int argc, char *argv[]){
	progname = argv[0];
	extern int optind;
	int option = 0,
	    fracprec = 0;
	register int c = 0,
		 nbuf = 0;
	register float count = 0;
	char *format = NULL,
		*separator = "";
	
	while ( (option = getopt(argc, argv, ":p:s:w")) != -1 ){
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
	separator = ( strncmp(separator, "", sizeof(char *)) == 0 )
			? "\n" 
			: separator;

	for ( count = start; count <= stop; count += step ) {
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

char *buildfmt() {
	int precision = 0;
	char *picture = NULL,
	     *fmtbuf = NULL; 
	
	if ((fmtbuf = calloc(16, sizeof(char *))) == NULL){
		pfmt(stderr, MM_ERROR, "%s: could not allocate an "
			"array of %d elements, each one "
			"being %lu bytes large.\n",
			progname, 16, (sizeof(char *)));
		exit(1);
	}

	/* Default. */
	if (!fPicture && !fWadding) {
		snprintf(fmtbuf, sizeof(fmtbuf), "%s", "%g%s");
		return fmtbuf;
	} 
	
	if (fPicture || fWadding) { 
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
		free(picture);

		if ( precision == -1) {
			pfmt(stderr, MM_ERROR,
				"%s: picture '%s' is not a number.\n",
				progname, picstr);
			exit(1);
		}

		snprintf(fmtbuf, sizeof(fmtbuf), "%%.%df%%s", precision);
		return fmtbuf;
	}
}

char *getlgstr() {
	register float c = 0;
	char strflt[32] = {(char)0},
	     *lgstnum;
	size_t longlength = 0,
	       curlength = 0;

	if ((lgstnum = calloc(sizeof(strflt), sizeof(char *))) == NULL){
		pfmt(stderr, MM_ERROR, "%s: could not allocate an "
			"array of %d elements, each one "
			"being %lu bytes large.\n",
			progname, sizeof(strflt), (sizeof(char *)));
		exit(1);
	}

	for (c=start; c <= stop; c += step) {
		sprintf(strflt, "%g", c);
		curlength = strlen(strflt);
		if (curlength > longlength) {
			longlength = curlength;
			lgstnum = strdup(strflt);
		}
	}

	return lgstnum;
}

int afterdecsep(char *s) {
	register int c = 0;
	int fraclen = 0;
	char *fracpart = NULL;

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
		       if (s[c] == '.') {
				fracpart = &s[(c + 1)];
				s[c] = '\0';
				break;
		       }
		}
		fraclen = strlen(fracpart);
	}

	return fraclen;
}

void usage(void) {
	pfmt(stderr, MM_NOSTD,
		"usage: %s: [-w] [-ppicture] [-sseparator] [start [step]] stop\n",
		progname);
	exit(1);
}
