/* 
 * wcputs.c - Implements puts() for wide character
 *
 * Copyright (C) 2024 Luiz Antônio Rangel
 *
 * SPDX-Licence-Identifier: Zlib
 */

#include <stdio.h>
#include <wchar.h>

int putws(const wchar_t *s) {
	register unsigned int c = 0;
	register wint_t r = '\0';
	for (; s[c]; c++) {
		r = putwc(s[c], stdout);
		switch (r) {
			case WEOF:
				return r;
		}
	}
	r = putwc(L'\n', stdout);
	return r;
}
