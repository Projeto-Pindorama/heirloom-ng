/*
 * yes.c - repeatedly print a string or "y" if no arguments
 *
 * Rewritten from scratch, `main` takes up 10 less instructions when compiled
 *   with gcc -O3 and should also be faster
 */
/*
 * Copyright (C) 2022: Arthur Bacci (arthurbacci)
 *
 * SPDX-Licence-Identifier: Zlib
 */

#include <stdio.h>

int main(int argc, char **argv) {
	// Defining it before looping for the comparison to be executed a single
	// time
	const char *to_print = argc > 1 ? argv[1] : "y";

	while (puts(to_print) != EOF);

	// Always return error since it can only stop if it couldn't write
	return 1;
}
