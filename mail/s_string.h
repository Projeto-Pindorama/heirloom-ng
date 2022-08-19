/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*
 * Copyright (c) 1999, by Sun Microsystems, Inc.
 * All rights reserved.
 */

/* This is a private header file.				*/

/*	from OpenSolaris "s_string.h	1.6	05/06/08 SMI" 	 SVr4.0 1.4		*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)s_string.h	1.3 (gritter) 6/18/05
 */
/* extensible strings */

#ifndef _S_STRING_H
#define _S_STRING_H

#include <string.h>

typedef struct string {
	char *base;	/* base of string */
	char *end;	/* end of allocated space+1 */
	char *ptr;	/* ptr into string */
} string;

/*
 * LINT:  expect lint warnings from the following macros
 * Some macros have an integer at the end, which has null effect according
 * to lint, but causes the last expression to be evaluated as an int (didn't
 * change this).
 */
#define s_clone(s)	s_copy((s)->ptr)
#define s_curlen(s)	((s)->ptr - (s)->base)
#define s_dup(s)	s_copy((s)->base)
#define s_getc(s)	(*((s)->ptr++))
#define s_peek(s)	(*((s)->ptr))
#define s_putc(s,c)     (((s)->ptr < (s)->end) ? (*((s)->ptr)++ = (char)(c)) : s_grow((s),(c)))
#define s_reset(s)	((s) ? (*((s)->ptr = (s)->base) = '\0' , (s)) : s_new())
#define s_restart(s)	((s)->ptr = (s)->base , (s))
#define s_skipc(s)	((s)->ptr++)
#define s_space(s)	((s)->end - (s)->base)
#define s_terminate(s)  (((s)->ptr < (s)->end) ? (*(s)->ptr = 0) : (s_grow((s),0), (s)->ptr--, 0))
#define s_to_c(s)	((s)->base)
#define s_ptr_to_c(s)	((s)->ptr)

extern string *s_append(string *to, char *from);
extern string *s_array(char *, size_t len);
extern string *s_copy(char *);
extern void s_free(string*);
extern int s_grow(string *sp, int c);
extern string *s_new(void);
extern string *s_parse(string *from, string *to);
extern char *s_read_line(FILE *fp, string *to);
extern size_t s_read_to_eof(FILE *fp, string *to);
extern string *s_seq_read(FILE *fp, string *to, int lineortoken);
extern void s_skipwhite(string *from);
extern string *s_tok(string*, char*);
extern void s_tolower(string*);

/* controlling the action of s_seq_read */
#define TOKEN 0		/* read the next whitespace delimited token */
#define LINE 1		/* read the next logical input line */
#define s_getline(a,b) s_seq_read(a,b,LINE)
#define s_gettoken(a,b) s_seq_read(a,b,TOKEN)

#endif /* _S_STRING_H */
