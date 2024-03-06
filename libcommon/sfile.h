/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * SPDX-Licence-Identifier: Zlib
 */
/*	Sccsid @(#)sfile.h	1.4 (gritter) 4/17/03	*/

/*
 * Return values:
 *
 * src_size	The entire range has been copied. The file offset of both
 *              dst_fd	and src_fd have been set to this position. The
 *              operation has been completed successfully.
 *
 * >0		Number of bytes written. The file offset of both dst_fd
 * 		and src_fd have been set to this position. The operation
 * 		may continue using read()/write().
 *
 * 0		No data was written; operation may continue.
 *
 * -1		An error occured; operation may not continue.
 */
extern long long	sfile(int dst_fd, int src_fd, mode_t src_mode,
				long long src_size);
