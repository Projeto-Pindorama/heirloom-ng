/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


/*      Portions Copyright(c) 1988, Sun Microsystems, Inc.      */
/*      All Rights Reserved.                                    */

/*
 * Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)helper.c	1.2 (gritter) 6/29/05
 */
/* from OpenSolaris "hashserv.c	1.12	05/06/08 SMI"	 SVr4.0 1.10.5.1 */
/*
 *	UNIX shell
 */

#include	"defs.h"
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<errno.h>
#include 	<ctype.h>

/*
 * Determine if file given by name is accessible with permissions
 * given by mode.
 * Regflag argument non-zero means not to consider 
 * a non-regular file as executable. 
 */

int
chk_access(const char *name, mode_t mode, int regflag)
{	
	static int flag;
	static uid_t euid; 
	struct stat statb;
	mode_t ftype;
	
	if(flag == 0) {
		euid = geteuid();
		flag = 1;
	}
	ftype = statb.st_mode & S_IFMT;
	if (stat((char *)name, &statb) == 0) {
		ftype = statb.st_mode & S_IFMT;
		if(mode == S_IEXEC && regflag && ftype != S_IFREG)
			return(2);
		if(access((char *)name, mode>>6) == 0) {
			if(euid == 0) {
				if (ftype != S_IFREG || mode != S_IEXEC)
					return(0);
		    		/* root can execute file as long as it has execute 
			   	permission for someone */
				if (statb.st_mode & (S_IEXEC|(S_IEXEC>>3)|(S_IEXEC>>6)))
					return(0);
				return(3);
			}
			return(0);
		}
	}
	return(errno == EACCES ? 3 : 1);
}

long long
stoifll(const char *icp)
{
	const unsigned char	*cp;
	long long	r = 0;
	int		sign = 1;
	unsigned char	c;

	for (cp = (unsigned char *)icp; *cp == ' '; cp++);
	if (*cp == '-') {
		sign = -1;
		cp++;
	} else if (*cp == '+')
		cp++;
	while ((c = *cp, isdigit(c)) && c) {
		r = r * 10 + c - '0';
		cp++;
	}
	return (r * sign);
}

void
failed(const char *p, const char *q)
{
	fprintf(stderr, "%s: %s\n", p, q);
	exit(ERROR);
}
