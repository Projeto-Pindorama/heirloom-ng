/*
 * afterchar.c - print character position at string.
 *
 * Copyright (c) 2024 Luiz Antônio Rangel (takusuman)
 *
 * SPDX-Licence-Identifier: Zlib
 */

int afterchar(const char p[], int a) {
	unsigned int pos = 0;

	for (;; pos++) {
		if (p[pos] == a) {
			break;
		} else if (p[pos] == '\0') {
			pos = 0;
			break;
		}
	}

	return pos;
}
