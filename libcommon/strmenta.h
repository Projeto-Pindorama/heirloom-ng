/*
 * strmenta.h - header for string utilities
 *
 * Copyright (c) 2024 Luiz Antônio Rangel (takusuman)
 *
 * SPDX-Licence-Identifier: Zlib
 */

#include "sputs.h"

/* Boilerplate for 'pointer++; pointerc--;' */
#define shift(p, d)	p++; d--

extern char *basename(const char path[]);
extern int afterchar(const char s[], int c);
