/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */
/*	Sccsid @(#)msgselect.h	1.2 (gritter) 9/21/03	*/

#define	MSG_LEVEL	0

#if MSG_LEVEL == 1
#define	msgselect(a, b)	a
#else
#define	msgselect(a, b)	b
#endif
