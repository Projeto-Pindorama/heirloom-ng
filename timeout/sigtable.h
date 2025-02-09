/*
 * sigtable.h - declarations for a signal map-like struct.
 */
/*
 * Partially derived from kill/strsig.c
 *
 * Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 * Copyright (C) 2023, 2024: Luiz Antônio (takusuman)
 *
 * SPDX-Licence-Identifier: CDDL-1.0
 */


struct sig_strlist {
	int   signum;
	char *signame;
};

extern const struct sig_strlist sig_strs[];
