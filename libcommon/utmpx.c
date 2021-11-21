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
/*	Sccsid @(#)utmpx.c	1.12 (gritter) 1/22/06	*/

#include <stdio.h>

#if defined (__FreeBSD__) || defined (__dietlibc__) || defined (__NetBSD__) || \
	defined (__UCLIBC__) || defined (__OpenBSD__) || \
	defined (__DragonFly__) || defined (__APPLE__)
#include <sys/types.h>
#include <sys/time.h>
#include <utmp.h>
#include <string.h>

#include "utmpx.h"

static FILE	*utfp;
static struct utmpx	utx;
static const char	*utmpfile = _PATH_UTMP;

static FILE *
init(void)
{
	if (utfp == NULL && (utfp = fopen(utmpfile, "r+")) == NULL)
		if ((utfp = fopen(utmpfile, "r")) == NULL)
			return NULL;
	return utfp;
}

static struct utmpx *
utmp2utmpx(struct utmpx *ux, const struct utmp *up)
{
#ifndef	__dietlibc__
	memset(ux, 0, sizeof *ux);
	ux->ut_tv.tv_sec = up->ut_time;
	memcpy(ux->ut_line, up->ut_line, UT_LINESIZE);
	memcpy(ux->ut_user, up->ut_name, UT_NAMESIZE);
	memcpy(ux->ut_host, up->ut_host, UT_HOSTSIZE);
	if (strcmp(up->ut_line, "~") == 0)
		ux->ut_type = BOOT_TIME;
	else if (strcmp(up->ut_line, "|") == 0)
		ux->ut_type = OLD_TIME;
	else if (strcmp(up->ut_line, "}") == 0)
		ux->ut_type = NEW_TIME;
	else if (*up->ut_name == 0)
		ux->ut_type = DEAD_PROCESS;
	else
		ux->ut_type = USER_PROCESS;
#else	/* __dietlibc__ */
	*ux = *up;
#endif	/* __dietlibc__ */
	return ux;
}

static struct utmp *
utmpx2utmp(struct utmp *up, const struct utmpx *ux)
{
#ifndef	__dietlibc__
	memset(up, 0, sizeof *up);
	up->ut_time = ux->ut_tv.tv_sec;
	switch (ux->ut_type) {
	case DEAD_PROCESS:
		memcpy(up->ut_line, ux->ut_line, UT_LINESIZE);
		break;
	default:
	case EMPTY:
	case INIT_PROCESS:
	case LOGIN_PROCESS:
	case RUN_LVL:
	case ACCOUNTING:
		return NULL;
	case BOOT_TIME:
		strcpy(up->ut_name, "reboot");
		strcpy(up->ut_line, "~");
		break;
	case OLD_TIME:
		strcpy(up->ut_name, "date");
		strcpy(up->ut_line, "|");
		break;
	case NEW_TIME:
		strcpy(up->ut_name, "date");
		strcpy(up->ut_line, "{");
		break;
	case USER_PROCESS:
		memcpy(up->ut_line, ux->ut_line, UT_LINESIZE);
		memcpy(up->ut_name, ux->ut_user, UT_NAMESIZE);
		memcpy(up->ut_host, ux->ut_host, UT_HOSTSIZE);
	}
#else	/* __dietlibc__ */
	*up = *ux;
#endif	/* __dietlibc__ */
	return up;
}

struct utmpx *
getutxent(void)
{
	static struct utmp	zero;
	struct utmp	ut;

	if (init() == NULL)
		return NULL;
	do {
		if (fread(&ut, sizeof ut, 1, utfp) != 1)
			return NULL;
	} while (memcmp(&ut, &zero, sizeof ut) == 0);
	return utmp2utmpx(&utx, &ut);
}

struct utmpx *
getutxline(const struct utmpx *ux)
{
	struct utmp	ut;

	if (init() == NULL)
		return NULL;
	fseek(utfp, 0, SEEK_SET);
	while (fread(&ut, sizeof ut, 1, utfp) == 1) {
		utmp2utmpx(&utx, &ut);
		if ((utx.ut_type == LOGIN_PROCESS ||
					utx.ut_type == USER_PROCESS) &&
				strcmp(ut.ut_line, utx.ut_line) == 0)
			return &utx;
	}
	return NULL;
}

struct utmpx *
getutxid(const struct utmpx *ux)
{
#ifdef	__dietlibc__
	struct utmp	ut;
#endif

	if (init() == NULL)
		return NULL;
#ifdef	__dietlibc__
	fseek(utfp, 0, SEEK_SET);
	while (fread(&ut, sizeof ut, 1, utfp) == 1) {
		utmp2utmpx(&utx, &ut);
		switch (ux->ut_type) {
		case BOOT_TIME:
		case OLD_TIME:
		case NEW_TIME:
			if (ux->ut_type == utx.ut_type)
				return &utx;
			break;
		case INIT_PROCESS:
		case LOGIN_PROCESS:
		case USER_PROCESS:
		case DEAD_PROCESS:
			if (ux->ut_type == utx.ut_type &&
					ux->ut_id == utx.ut_id)
				return &utx;
			break;
		}
	}
#endif	/* __dietlibc__ */
	return NULL;
}

void
setutxent(void)
{
	if (init() == NULL)
		return;
	fseek(utfp, 0, SEEK_SET);
}

void
endutxent(void)
{
	FILE	*fp;

	if (init() == NULL)
		return;
	fp = utfp;
	utfp = NULL;
	fclose(fp);
}

int
utmpxname(const char *name)
{
	utmpfile = strdup(name);
	return 0;
}

extern struct utmpx *
pututxline(const struct utmpx *up)
{
	struct utmp	ut;
	struct utmpx	*rp;

	if (init() == NULL)
		return NULL;
	/*
	 * Cannot use getutxid() because there is no id field. Use
	 * the equivalent of getutxline() instead.
	 */
	while (fread(&ut, sizeof ut, 1, utfp) == 1) {
		if (strncmp(ut.ut_line, up->ut_line, UT_LINESIZE) == 0) {
			fseek(utfp, -sizeof ut, SEEK_CUR);
			break;
		}
	}
	fflush(utfp);
	if (utmpx2utmp(&ut, up) == NULL)
		rp = NULL;
	else if (fwrite(&ut, sizeof ut, 1, utfp) == 1) {
		utx = *up;
		rp = &utx;
	} else
		rp = NULL;
	fflush(utfp);
	return rp;
}

extern void
updwtmpx(const char *name, const struct utmpx *up)
{
	FILE	*fp;

	if ((fp = fopen(name, "a")) == NULL)
		return;
	fwrite(up, sizeof *up, 1, fp);
	fclose(fp);
}

#endif	/* __FreeBSD__ || __dietlibc__ || __NetBSD__ || __UCLIBC__ ||
	 	__OpenBSD__ || __DragonFly__ || __APPLE__ */
