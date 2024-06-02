/* 
 * wcputs.h - Implements puts() for wide character
 *
 * Copyright (C) 2024 Luiz Antônio Rangel
 *
 * SPDX-Licence-Identifier: Zlib
 */


#if !defined(sun) || !defined(hpux) || !defined(_AIX)
#include <stddef.h>
int putws(const wchar_t *s);
#else
#include <stdio.h>
#include <widec.h>
#endif
