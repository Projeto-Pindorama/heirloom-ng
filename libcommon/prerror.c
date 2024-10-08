/* prerror - wrapper function for pfmt.h */
/* This basically prints both errno and string errors.
 * I hope my explanation isn't so confusing. 
 */
/* Copyright (C) 2022: Arthur Bacci (arthurbacci)
 *
 * SPDX-Licence-Identifier: Zlib
 */

#include <errno.h>
#include <string.h>
#include "pfmt.h"

void prerror(int error) {
	errno = 0;
	char *str = strerror(error);
	if (errno != 0)
		pfmt(stderr, MM_ERROR, "errno %d\n", error);
	else
		pfmt(stderr, MM_ERROR, "%s\n", str);
}
