/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*	from OpenSolaris "hash.h	1.9	05/06/08 SMI"	 SVr4.0 1.2		*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)hash.h	2.2 (gritter) 6/21/05
 */

#include <inttypes.h>

#define	HASHWIDTH 27
#define	HASHSIZE 134217689L	/* prime under 2^HASHWIDTH */
#define	INDEXWIDTH 9
#define	INDEXSIZE (1<<INDEXWIDTH)
#define	NI (INDEXSIZE+1)
#define	ND ((25750/2) * sizeof (*table))
#define	BYTE 8

extern uint32_t *table;
#undef	index
#define	index	sp_index
extern int32_t index[];	/* into dif table based on hi hash bits */

void	hashinit(void);
int	hashlook(const char *);
uint32_t	hash(const char *);
