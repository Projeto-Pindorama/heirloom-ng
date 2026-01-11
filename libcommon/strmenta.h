/*
 * strmenta.h - header for string utilities
 *
 * Copyright (c) 2025 Luiz Antônio Rangel (takusuman)
 *
 * SPDX-Licence-Identifier: Zlib
 */

#include "sputs.h"

/* Boilerplate for 'pointer++; pointerc--;' */
#define shift(p, d)	p++; d--

extern char *basename(const char path[]);
extern char *ssafe(const char *s);
extern char *strjoin(char *elems[], char *sep);