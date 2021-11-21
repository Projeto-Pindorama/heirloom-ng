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
/*	Sccsid @(#)sfile.c	1.9 (gritter) 6/7/04	*/

#ifdef	__linux__
#undef	_FILE_OFFSET_BITS

#include	<sys/types.h>
#include	<sys/sendfile.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<limits.h>
#include	<errno.h>
#include	"sfile.h"

long long
sfile(int dfd, int sfd, mode_t mode, long long count)
{
	static int	enosys, einval, success;
	off_t	offset;
	ssize_t	sent, total;
	extern void	writerr(void *, int, int);
	/*
	 * A process is not interruptible while executing a sendfile()
	 * system call. So it is not advisable to to send an entire
	 * file with one call; it is sent in parts so signals can
	 * be delivered in between.
	 */
	const ssize_t	chunk = 196608;

	/*
	 * If a previous call returned ENOSYS, the operating system does
	 * not support sendfile() at all and it makes no sense to try it
	 * again.
	 *
	 * If a previous call returned EINVAL and there was no successful
	 * call yet, it is very likely that this is a permanent error
	 * condition (on Linux 2.6.0-test4, sendfile() may be used for
	 * socket targets only; older versions don't support tmpfs as
	 * source file system etc.).
	 */
	if (enosys || !success && einval ||
			(mode&S_IFMT) != S_IFREG || count > SSIZE_MAX)
		return 0;
	offset = lseek(sfd, 0, SEEK_CUR);
	sent = 0, total = 0;
	while (count > 0 && (sent = sendfile(dfd, sfd, &offset,
					count > chunk ? chunk : count)) > 0) {
		count -= sent, total += sent;
	}
	if (total && lseek(sfd, offset, SEEK_SET) == (off_t)-1)
		return -1;
	if (count == 0 || sent == 0) {
		success = 1;
		return total;
	}
	switch (errno) {
	case ENOSYS:
		enosys = 1;
		return 0;
	case EINVAL:
		einval = 1;
		return 0;
	case ENOMEM:
		return 0;
	default:
		writerr(NULL, count > chunk ? chunk : count, 0);
		return -1;
	}
}
#else	/* !__linux__ */
#include	<sys/types.h>

/*ARGSUSED*/
long long
sfile(int dfd, int sfd, mode_t mode, long long count)
{
	return 0;
}
#endif	/* __linux__ */
