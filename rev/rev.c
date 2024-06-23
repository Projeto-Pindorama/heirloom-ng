/*
 * rev.c - reverse lines of a file 
 *
 * A little bit based on Luiz' implementation
 */
/*
 * Copyright (C) 2024: Luiz Antônio Rangel (takusuman)
 * Copyright (C) 2024: Samuel Brederodes (callsamu)
 *
 * SPDX-Licence-Identifier: Zlib
 */

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NULL_BYTE '\0'

static char *progname;

int copy_line(FILE * from, char * to_buffer, size_t to_buffer_size) {
    for (int i = 0; i < to_buffer_size; i++) {
        int c = fgetc(from);

        switch (c) {
        case EOF:
            to_buffer[i] = NULL_BYTE;
            return EOF;
        case '\n':
            to_buffer[i] = NULL_BYTE;
            return 0;
        default:
            to_buffer[i] = c;
        }
    }

    return 0;
}

void reverse_string(char * string) {
    int len = strlen(string);

    for (int i = 0; i < len / 2; i++) {
        char tmp = string[i];
        string[i] = string[len - i - 1];
        string[len - i - 1] = tmp;
    }
}

int main(int argc, char * argv[]) {
    progname = argv[0];

    int fdinput = STDIN_FILENO;
    if (argc > 1) {
        fdinput = open(argv[1], O_RDONLY);

        if (fdinput < 0) {
            fprintf(stderr, "%s: %s\n", progname, strerror(errno));
            return EXIT_FAILURE;
        }
    }

    FILE * input = fdopen(fdinput, "r");
    if (!input) {
        fprintf(stderr, "%s: cannot open stream at descriptor %d: %s\n", progname, fdinput, strerror(errno));
        return EXIT_FAILURE;
    }

    char buffer[BUFSIZ];

    while (1) {
        int eof = copy_line(input, buffer, BUFSIZ);
        reverse_string(buffer);
        puts(buffer);
        strncpy(buffer, "", BUFSIZ);

        if (eof == EOF) {
            if (fdinput) {
                fclose(input);
            }
            return EXIT_SUCCESS;
        }
    }
}
