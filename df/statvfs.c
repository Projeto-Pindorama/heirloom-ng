/*
 * Copyright (c) 2004 Gunnar Ritter
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
/*	Sccsid @(#)statvfs.c	1.6 (gritter) 1/22/06	*/

typedef	unsigned long long	fsblkcnt_t;
typedef	unsigned long long	fsfilcnt_t;

struct statvfs {
	unsigned long	f_bsize;
	unsigned long	f_frsize;
	fsblkcnt_t	f_blocks;
	fsblkcnt_t	f_bfree;
	fsblkcnt_t	f_bavail;
	fsfilcnt_t	f_files;
	fsfilcnt_t	f_ffree;
	fsfilcnt_t	f_favail;
	unsigned long	f_fsid;
	unsigned long	f_flag;
	unsigned long	f_namemax;
};

#if defined (__dietlibc__)
#include <sys/vfs.h>
#elif defined (__NetBSD__) || defined (__FreeBSD__) || defined (__OpenBSD__) \
	|| defined (__APPLE__)
#include <sys/param.h>
#include <sys/mount.h>
#endif

int
statvfs(const char *path, struct statvfs *buf)
{
	struct statfs	sf;

	if (statfs(path, &sf) < 0)
		return -1;
	buf->f_bsize = buf->f_frsize = sf.f_bsize;
	buf->f_blocks = sf.f_blocks;
	buf->f_bfree = sf.f_bfree;
	buf->f_bavail = sf.f_bavail;
	buf->f_files = sf.f_files;
	buf->f_ffree = buf->f_favail = sf.f_ffree;
	buf->f_fsid = 0;
#if !defined (__NetBSD__) && !defined (__FreeBSD__) && !defined (__OpenBSD__) \
		&& !defined (__APPLE__)
	buf->f_namemax = sf.f_namelen;
#else	/* __NetBSD__, __FreeBSD__, __OpenBSD__, __APPLE__ */
	buf->f_namemax = PATH_MAX;
#endif	/* __NetBSD__, __FreeBSD__, __OpenBSD__, __APPLE__ */
	return 0;
}
