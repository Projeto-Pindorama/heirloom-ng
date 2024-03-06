/*
 * SPDX-Licence-Identifier: CDDL-1.0
 */
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/


#ifndef	_FATAL_H
#define	_FATAL_H

/*	from OpenSolaris "fatal.h	1.9	05/06/08 SMI"	 SVr4.0 1.4.1.1 	*/

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)fatal.h	1.4 (gritter) 12/25/06
 */

#ifdef	__cplusplus
extern "C" {
#endif

extern	int	Fflags;
extern	char	*Ffile;
extern	int	Fvalue;
extern	int	Fjmp[10];

#define	FTLMSG		0100000
#define	FTLCLN		0040000
#define	FTLFUNC		0020000
#define	FTLACT		0000077
#define	FTLJMP		0000002
#define	FTLEXIT		0000001
#define	FTLRET		0000000

#define	FSAVE(val)	SAVE(Fflags, old_Fflags); Fflags = val;
#define	FRSTR()	RSTR(Fflags, old_Fflags);

#ifdef	__cplusplus
}
#endif

#endif	/* _FATAL_H */
