/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute
 * it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 */
/*	Sccsid @(#)getdir.c	1.20 (gritter) 5/14/06	*/

#ifndef	__linux__
/*
 * 32-bit Solaris and Open UNIX do not have 64-bit getdents(); but
 * having _FILE_OFFSET_BITS=64 will make it use a dirent64 struct
 * on Open UNIX -> SEGV.
 */
#undef	_FILE_OFFSET_BITS
#endif	/* !__linux__ */

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<string.h>
#include	<dirent.h>
#include	<unistd.h>

#if defined (__UCLIBC__)
#include <linux/types.h>
#include <linux/dirent.h>
#define	getdents(a, b, c)	__getdents64(a, b, c)
#define	dirent	dirent64
extern int	getdents(int, struct dirent *, size_t);
#elif defined	(__GLIBC__) || defined (__FreeBSD__) || defined (_AIX) || \
	defined (__NetBSD__) || defined (__OpenBSD__) || \
	defined (__DragonFly__) || defined (__APPLE__)
#include	<dirent.h>
#define	getdents(a, b, c)	getdirentries((a), (char *)(b), (c), &(db->g_offs))
#if defined (__FreeBSD__) || defined (__NetBSD__) || defined (__OpenBSD__) || \
	defined (__DragonFly__) || defined (__APPLE__)
#undef	d_ino
#endif	/* __FreeBSD__ || __NetBSD__ || __OpenBSD__ || __DragonFly__
	 || __APPLE__ */
#elif defined	(__dietlibc__)
#include	<dirent.h>
#include	<unistd.h>
#else		/* !__GLIBC__, !__dietlibc__ */
#ifdef	__hpux
#define		_KERNEL
#endif	/* __hpux */
/*#include	<sys/dirent.h>*/
#ifdef		__hpux
#ifndef	_INO64_T
typedef	unsigned long long	uint64_t;
typedef	uint64_t	ino64_t;
#endif	/* !_INO64_T */
#ifdef	__LP64__
#define	dirent		__dirent64
#else	/* !__LP64__ */
#define	dirent		__dirent32
#endif	/* !__LP64__ */
#define	d_reclen	__d_reclen
#define	d_name		__d_name
#define	d_ino		__d_ino
#endif		/* __hpux */
#endif		/* !__GLIBC__, !__dietlibc__ */

#include	"getdir.h"

#define	DIBSIZE	5120

struct	getdb {
#if !defined (__FreeBSD__) && !defined (__NetBSD__) && !defined (__OpenBSD__) \
		&& !defined (__DragonFly__) && !defined (__APPLE__)
	off_t		g_offs;
#else	/* __FreeBSD__, __NetBSD__, __OpenBSD__, __DragonFly__, __APPLE__ */
	long		g_offs;
#endif	/* __FreeBSD__, __NetBSD__, __OpenBSD__, __DragonFly__, __APPLE__ */
	struct dirent	*g_dirp;
	const char	*g_path;
	struct direc	g_dic;
	union {
		char		g_dirbuf[DIBSIZE+1];
		struct dirent	g_dummy[1];
	} g_u;
	int		g_num;
	int		g_fd;
};

struct getdb *
getdb_alloc(const char *path, int fd)
{
	struct getdb	*db;

	if ((db = malloc(sizeof *db)) == NULL)
		return NULL;
	db->g_dirp = NULL;
	db->g_offs = 0;
	db->g_fd = fd;
	db->g_path = path;
	return db;
}

void
getdb_free(struct getdb *db)
{
	free(db);
}

struct direc *
getdir(struct getdb *db, int *err)
{
	int	reclen;

	*err = 0;
	while (db->g_dirp == NULL)
	{
		/*LINTED*/
		db->g_num = getdents(db->g_fd,
				(struct dirent *)db->g_u.g_dirbuf,
				DIBSIZE);
		if (db->g_num <= 0) {
			if (db->g_num < 0)
				*err = errno;
			db->g_offs = 0;
			return NULL;
		}
		/*LINTED*/
		db->g_dirp = (struct dirent *)db->g_u.g_dirbuf;
		while (db->g_dirp &&
#if !defined (__FreeBSD__) && !defined (__NetBSD__) && !defined (__OpenBSD__) \
		&& !defined (__DragonFly__) && !defined (__APPLE__)
				db->g_dirp->d_ino == 0
#else	/* __FreeBSD__, __NetBSD__, __OpenBSD__, __DragonFly__, __APPLE__ */
				(db->g_dirp->d_fileno == 0
#ifdef DT_WHT
				  || db->g_dirp->d_type == DT_WHT
#endif
				  )
#endif	/* __FreeBSD__, __NetBSD__, __OpenBSD__, __DragonFly__, __APPLE__ */
		      )
		{
		next:
#ifndef	__DragonFly__
			reclen = db->g_dirp->d_reclen;
#else
			reclen = _DIRENT_DIRSIZ(db->g_dirp);
#endif
			if ((db->g_num -= reclen) == 0 || reclen == 0)
				db->g_dirp = NULL;
			else
				db->g_dirp =
					/*LINTED*/
					(struct dirent *)((char *)db->g_dirp
						+ reclen);
		}
	}
#if !defined (__FreeBSD__) && !defined (__NetBSD__) && !defined (__OpenBSD__) \
		&& !defined (__DragonFly__) && !defined (__APPLE__)
	if (db->g_dirp->d_ino == 0)
		goto next;
	db->g_dic.d_ino = db->g_dirp->d_ino;
#else	/* __FreeBSD__, __NetBSD__, __OpenBSD__, __DragonFly__, __APPLE__ */
	if (db->g_dirp->d_fileno == 0
#ifdef DT_WHT
	    || db->g_dirp->d_type == DT_WHT
#endif
	    )
	{
		goto next;
	}
	db->g_dic.d_ino = db->g_dirp->d_fileno;
#endif	/* __FreeBSD__, __NetBSD__, __OpenBSD__, __DragonFly__, __APPLE__ */
	db->g_dic.d_name = db->g_dirp->d_name;
#ifndef	__DragonFly__
		reclen = db->g_dirp->d_reclen;
#else
		reclen = _DIRENT_DIRSIZ(db->g_dirp);
#endif
	if ((db->g_num -= reclen) == 0 || reclen == 0)
		db->g_dirp = NULL;
	else
		/*LINTED*/
		db->g_dirp = (struct dirent *)((char *)db->g_dirp + reclen);
	return &(db->g_dic);
}
