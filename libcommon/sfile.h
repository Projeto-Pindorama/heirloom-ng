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
