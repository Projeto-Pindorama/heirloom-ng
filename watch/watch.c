/* 
 * watch.c - Keep an eye on a command output
 */
/*
 * Copyright (C) 2023: Luiz Antônio Rangel (takusuman)
 *
 * SPDX-Licence-Identifier: Zlib
 *
 */

#include <curses.h>
#include <pfmt.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

char *progname;
int main(int argc, char *argv[]);
int _exec(char program[]);
int _system(char program[]);
void usage(void);

struct Flag {
	int Beep_on_error, No_title, Use_exec;
}; struct Flag flag;

int main(int argc, char *argv[]) {
	progname = argv[0];
	int option, interval;
	
	while ( (option = getopt(argc, argv, "n:hbtx")) != -1 ) {
			switch (option) {
				case 'n':
					interval = atoi(optarg);
					break;
				case 'b':
					flag.Beep_on_error = 1;
					break;
				case 't':
					flag.No_title = 1;
					break;
				case 'x':
					// Uses exec() instead of system()
					flag.Use_exec = 1;
					break;
				case '?':
					pfmt(stderr, MM_ERROR,
						"unknown option: %c\n", optopt);
					usage();
				case 'h':
				default:
					usage();
			}
	}
	argc -= optind;
	argv += optind;
	// Missing operand
	if ( argc < 1 ){
		usage();
	}

	// Init curses screen
	initscr();

	// Function pointer, so we won't need to be checking if we're using
	// exec() or system() everytime.
	int (*execute)(char[]);
	if ( flag.Use_exec ) {
		execute = &_exec;
	} else {
		execute = &_system;
	}

	for (;;) {
		printf("Intervalo é: %d\n", interval);
		execute(argv[0]);
		sleep( (uint)(interval) );
		clear();
		refresh();
	}
	exit(0);
}

// Just because we doesn't want to be checking every cicle
int _exec(char program[]){
	execvp(program, &program);
}

int _system(char program[]){
	system(program);
}

void usage(void) {
	pfmt(stderr, MM_NOSTD,
		"usage: %s -n [seconds] [-btx] [command [args...]]\n", progname);
	exit(1);
}
