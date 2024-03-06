/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */
/*	Sccsid @(#)oblok.h	1.3 (gritter) 4/17/03	*/

#include	<sys/types.h>

#ifndef	OBLOK
enum	{
	OBLOK = 4096
};
#endif	/* !OBLOK */

enum	ob_mode {
	OB_EBF = 0,		/* error or mode unset */
	OB_NBF = 1,		/* not buffered */
	OB_LBF = 2,		/* line buffered */
	OB_FBF = 3		/* fully buffered */
};

struct	oblok {
	char	ob_blk[OBLOK];		/* buffered data */
	long long	ob_wrt;			/* amount of data written */
	int	ob_pos;			/* position of first empty date byte */
	int	ob_fd;			/* file descriptor to write to */
	enum ob_mode	ob_bf;		/* buffering mode */
};

/*
 * Allocate an output buffer with file descriptor fd and buffer mode bf.
 * If bf is OB_EBF, the choice is made dependant upon the file type.
 * NULL is returned if no memory is available.
 */
extern struct oblok	*ob_alloc(int fd, enum ob_mode bf);

/*
 * Deallocate the passed output buffer, flushing all data. The file
 * descriptor is not closed. Returns -1 if flushing fails.
 */
extern ssize_t	ob_free(struct oblok *op);

/*
 * Write data of length sz to the passed output buffer. Returns -1 on
 * error or the amount of data written.
 */
extern ssize_t	ob_write(struct oblok *op, const char *data, size_t sz);

/*
 * Flush all data in the passed output buffer. Returns -1 on error or
 * the amount of data written; 0 is success and means 'nothing to flush'.
 * The underlying device is not flushed (i. e. no fsync() is performed).
 */
extern ssize_t	ob_flush(struct oblok *op);

/*
 * Flush all output buffers. Called automatically using atexit(). Returns
 * -1 on error or the number of buffers flushed; 0 is success.
 */
extern int	ob_clear(void);

/*
 * putc() workalike.
 */
#define	ob_put(c, op)	((op)->ob_bf != OB_FBF || (op)->ob_pos >= (OBLOK) - 1 ?\
				ob_chr((c), (op)) : \
				(int)((op)->ob_blk[(op)->ob_pos++] = (char)(c)))


/*
 * fputc() workalike.
 */
extern int	ob_chr(int c, struct oblok *op);

/*
 * This function must be supplied by the calling code; it is called on
 * write error.
 */
extern void	writerr(struct oblok *op, int count, int written);
