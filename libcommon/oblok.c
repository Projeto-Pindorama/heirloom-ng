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
/*	Sccsid @(#)oblok.c	1.7 (gritter) 7/16/04	*/

#include	<sys/types.h>
#include	<unistd.h>
#include	<string.h>
#include	<errno.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<malloc.h>

#include	"memalign.h"
#include	"oblok.h"

struct	list {
	struct list	*l_nxt;
	struct oblok	*l_op;
};

static struct list	*bloks;
static int	exitset;

int
ob_clear(void)
{
	struct list	*lp;
	int	val = 0;

	for (lp = bloks; lp; lp = lp->l_nxt) {
		if (ob_flush(lp->l_op) < 0)
			val = -1;
		else if (val >= 0)
			val++;
	}
	return val;
}

static void
add(struct oblok *op)
{
	struct list	*lp, *lq;

	if ((lp = calloc(1, sizeof *lp)) != NULL) {
		lp->l_nxt = NULL;
		lp->l_op = op;
		if (bloks) {
			for (lq = bloks; lq->l_nxt; lq = lq->l_nxt);
			lq->l_nxt = lp;
		} else
			bloks = lp;
		if (exitset == 0) {
			exitset = 1;
			atexit((void (*)(void))ob_clear);
		}
	}
}

static void
del(struct oblok *op)
{
	struct list	*lp, *lq = NULL;

	if (bloks) {
		for (lp = bloks; lp && lp->l_op != op; lp = lp->l_nxt)
			lq = lp;
		if (lp) {
			if (lq)
				lq->l_nxt = lp->l_nxt;
			if (lp == bloks)
				bloks = bloks->l_nxt;
			free(lp);
		}
	}
}

struct oblok *
ob_alloc(int fd, enum ob_mode bf)
{
	static long	pagesize;
	struct oblok	*op;

	if (pagesize == 0)
		if ((pagesize = sysconf(_SC_PAGESIZE)) < 0)
			pagesize = 4096;
	if ((op = memalign(pagesize, sizeof *op)) == NULL)
		return NULL;
	memset(op, 0, sizeof *op);
	op->ob_fd = fd;
	switch (bf) {
	case OB_EBF:
		op->ob_bf = isatty(fd) ? OB_LBF : OB_FBF;
		break;
	default:
		op->ob_bf = bf;
	}
	add(op);
	return op;
}

ssize_t
ob_free(struct oblok *op)
{
	ssize_t	wrt;

	wrt = ob_flush(op);
	del(op);
	free(op);
	return wrt;
}

static ssize_t
swrite(int fd, const char *data, size_t sz)
{
	ssize_t	wo, wt = 0;

	do {
		if ((wo = write(fd, data + wt, sz - wt)) < 0) {
			if (errno == EINTR)
				continue;
			else
				return wt;
		}
		wt += wo;
	} while (wt < sz);
	return sz;
}

ssize_t
ob_write(struct oblok *op, const char *data, size_t sz)
{
	ssize_t	wrt;
	size_t	di, isz;

	switch (op->ob_bf) {
	case OB_NBF:
		wrt = swrite(op->ob_fd, data, sz);
		op->ob_wrt += wrt;
		if (wrt != sz) {
			op->ob_bf = OB_EBF;
			writerr(op, sz, wrt>0?wrt:0);
			return -1;
		}
		return wrt;
	case OB_LBF:
	case OB_FBF:
		isz = sz;
		while (op->ob_pos + sz > (OBLOK)) {
			di = (OBLOK) - op->ob_pos;
			sz -= di;
			if (op->ob_pos > 0) {
				memcpy(&op->ob_blk[op->ob_pos], data, di);
				wrt = swrite(op->ob_fd, op->ob_blk, (OBLOK));
			} else
				wrt = swrite(op->ob_fd, data, (OBLOK));
			op->ob_wrt += wrt;
			if (wrt != (OBLOK)) {
				op->ob_bf = OB_EBF;
				writerr(op, (OBLOK), wrt>0?wrt:0);
				return -1;
			}
			data += di;
			op->ob_pos = 0;
		}
		if (op->ob_bf == OB_LBF) {
			const char	*cp;

			cp = data;
			while (cp < &data[sz]) {
				if (*cp == '\n') {
					di = cp - data + 1;
					sz -= di;
					if (op->ob_pos > 0) {
						memcpy(&op->ob_blk[op->ob_pos],
								data, di);
						wrt = swrite(op->ob_fd,
							op->ob_blk,
							op->ob_pos + di);
					} else
						wrt = swrite(op->ob_fd,
							data, di);
					op->ob_wrt += wrt;
					if (wrt != op->ob_pos + di) {
						op->ob_bf = OB_EBF;
						writerr(op, di, wrt>0?wrt:0);
						return -1;
					}
					op->ob_pos = 0;
					data += di;
					cp = data;
				}
				cp++;
			}
		}
		if (sz == (OBLOK)) {
			wrt = swrite(op->ob_fd, data, sz);
			op->ob_wrt += wrt;
			if (wrt != sz) {
				op->ob_bf = OB_EBF;
				writerr(op, sz, wrt>0?wrt:0);
				return -1;
			}
		} else if (sz) {
			memcpy(&op->ob_blk[op->ob_pos], data, sz);
			op->ob_pos += sz;
		}
		return isz;
	case OB_EBF:
		;
	}
	return -1;
}

ssize_t
ob_flush(struct oblok *op)
{
	ssize_t	wrt = 0;

	if (op->ob_pos) {
		wrt = swrite(op->ob_fd, op->ob_blk, op->ob_pos);
		op->ob_wrt += wrt;
		if (wrt != op->ob_pos) {
			op->ob_bf = OB_EBF;
			writerr(op, op->ob_pos, wrt>0?wrt:0);
			wrt = -1;
		}
		op->ob_pos = 0;
	}
	return wrt;
}

int
ob_chr(int c, struct oblok *op)
{
	char	b;
	ssize_t	wrt;

	b = (char)c;
	wrt = ob_write(op, &b, 1);
	return wrt < 0 ? EOF : c;
}
