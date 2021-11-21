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
/*	Sccsid @(#)iblok.h	1.5 (gritter) 7/16/04	*/

/*
 * Functions to read a file sequentially.
 */

#include	<sys/types.h>		/* for off_t, pid_t */
#include	<stdio.h>		/* for EOF */
#include	<wchar.h>		/* for wchar_t */
#include	<limits.h>		/* for MB_LEN_MAX */

struct	iblok {
	long long	ib_endoff;	/* offset of endc from start of file */
	char	ib_mbuf[MB_LEN_MAX+1];	/* multibyte overflow buffer */
	char	*ib_mcur;		/* next byte to read in ib_mbuf */
	char	*ib_mend;		/* one beyond last byte in ib_mbuf */
	char	*ib_blk;		/* buffered data */
	char	*ib_cur;		/* next character in ib_blk */
	char	*ib_end;		/* one beyond last byte in ib_blk */
	int	ib_fd;			/* input file descriptor */
	int	ib_errno;		/* errno on error, or 0 */
	int	ib_incompl;		/* had an incomplete last line */
	int	ib_mb_cur_max;		/* MB_CUR_MAX at time of ib_alloc() */
	int	ib_seekable;		/* had a successful lseek() */
	pid_t	ib_pid;			/* child from ib_popen() */
	unsigned	ib_blksize;	/* buffer size */
};

/*
 * Allocate an input buffer with file descriptor fd. blksize may be
 * either the size of a buffer to allocate in ib_blk, or 0 if the
 * size is determined automatically. On error, NULL is returned and
 * errno indicates the offending error.
 */
extern struct iblok	*ib_alloc(int fd, unsigned blksize);

/*
 * Deallocate the passed input buffer. The file descriptor is not
 * closed.
 */
extern void		ib_free(struct iblok *ip);

/*
 * Open file name and do ib_alloc() on the descriptor.
 */
extern struct iblok	*ib_open(const char *name, unsigned blksize);

/*
 * Close the file descriptor in ip and do ib_free(). Return value is
 * the result of close().
 */
extern int		ib_close(struct iblok *ip);

/*
 * A workalike of popen(cmd, "r") using iblok facilities.
 */
extern struct iblok	*ib_popen(const char *cmd, unsigned blksize);

/*
 * Close an iblok opened with ib_popen().
 */
extern int		ib_pclose(struct iblok *ip);

/*
 * Read new input buffer. Returns the next character (or EOF) and advances
 * ib_cur by one above the bottom of the buffer.
 */
extern int		ib_read(struct iblok *ip);

/*
 * Get next character. Return EOF at end-of-file or read error.
 */
#define	ib_get(ip)	((ip)->ib_cur < (ip)->ib_end ? *(ip)->ib_cur++ & 0377 :\
				ib_read(ip))

/*
 * Unget a character. Note that this implementation alters the read buffer.
 * Caution: Calling this macro more than once might underflow ib_blk.
 */
#define ib_unget(c, ip)	(*(--(ip)->ib_cur) = (char)(c))

/*
 * Get file offset of last read character.
 */
#define	ib_offs(ip)	((ip)->ib_endoff - ((ip)->ib_end - (ip)->ib_cur - 1))

/*
 * Read a wide character using ib_get() facilities. *wc is used to store
 * the wide character, or WEOF if an invalid byte sequence was found.
 * The number of bytes consumed is stored in *len. Return value is the
 * corresponding byte sequence, or NULL at end-of-file in input.
 *
 * Note that it is not possible to mix calls to ib_getw() with calls to
 * ib_get(), ib_unget() or ib_seek() unless the last character read by
 * ib_getw() was L'\n'.
 */
extern char	*ib_getw(struct iblok *ip, wint_t *wc, int *len);

/*
 * Get a line from ip, returning the line length. Further arguments are either
 * the pointer to a malloc()ed buffer and a pointer to its size, or (NULL, 0)
 * if ib_getlin() shall allocate the buffer itselves. ib_getlin() will use
 * the realloc-style function reallc() to increase the buffer if necessary;
 * this function is expected never to fail (i. e., it must longjmp() or abort
 * if it cannot allocate a buffer of the demanded size).
 * On end-of-file or error, 0 is returned.
 */
extern size_t	ib_getlin(struct iblok *ip, char **line, size_t *alcd,
			void *(*reallc)(void *, size_t));

/*
 * Like lseek().
 */
extern off_t	ib_seek(struct iblok *ip, off_t off, int whence);
