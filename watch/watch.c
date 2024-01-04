/* 
 * watch.c - Keep an eye on a command output
 */
/*
 * Copyright (C) 2023: Luiz Antônio Rangel	(takusuman)
 * 		       Arthur Bacci		(arthurbacci)
 *
 * SPDX-Licence-Identifier: Zlib
 *
 * Support for calling commands with arguments without having to escape them
 * with double-dash thoroughly based of IIJ's iwatch(1).
 * As per the copyright header of IIJ's iwatch.c:
 * Copyright (c) 2000, 2001 Internet Initiative Japan Inc. 
 *
 * SPDX-Licence-Identifier: BSD-2-Clause
 */

#include <curses.h>
#include <errno.h>
#include <pfmt.h>
#include <prerror.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>
#include <wait.h>

static char *progname;

struct Flag {
	int Beep_on_error, No_title;
}; static struct Flag flag;

void usage(void) {
	pfmt(
		stderr, MM_NOSTD,
		"usage: %s [-n seconds] [-bt] command [args...]\n",
		progname
	);
	exit(1);
}

int main(int argc, char *argv[]) {
	progname = argv[0];
	int option = 0;
	/* Default interval of 2 seconds, as other major implementations
	usually do */
	int interval = 2;
	int c = 0, ec = 0, term_x = 0, term_y = 0;
	char **commandv;
	pid_t exec_pid;
	
	// Variables for the information header.
	// Defining nodename from now, since it shouldn't change while we're
	// watching the command.
	time_t now;
	struct tm *timeinfo;
	struct utsname u;
	if (uname(&u) == -1) {
		prerror(errno);
		exit(-1);
	}

	while ((option = getopt(argc, argv, "n:hbt")) != -1) {
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
		case 'h':
		default:
			usage();
		}
	}

	// FIXME: not good practise
	argc -= optind;
	argv += optind;
	
	// Missing operand
	if (argc < 1) usage();

	// Now we just have to copy the "rest" of argv[] to a new character
	// array allocating some space in memory with calloc(3) and then
	// copying using a for loop.
	
	if ((commandv = calloc((unsigned long)(argc + 1), sizeof(char *))) == NULL) {
		// Should i use prerror?
		perror("couldn't callocate");
		exit(-1);
	}
	
	for (c = 0; c < argc; c++) commandv[c] = argv[c];

	// Initialize curses terminal with colours to be used.
	// Get terminal size too, we're going to need it.
	newterm(getenv("TERM"), stdout, stdin);
	start_color();
	init_pair(1, COLOR_BLACK, COLOR_WHITE);

	for (;;) {
		// Clear terminal for the next cycle.
		clear();

		getmaxyx(stdscr, term_y, term_x);
		// Get current time to be passed as a string with ctime(3).
		time(&now);
		timeinfo = localtime(&now);

		if (!flag.No_title){
			char left[256], right[256], time[256];
			int left_len = 0, right_len = 0;

			attron(COLOR_PAIR(1) | A_BOLD);
			
			left_len = snprintf(
				left, 256, "Every %d second%s: %s",
				interval, interval == 1 ? "" : "s", argv[0]
			);
			
			// This is done because ctime returns a string with \n
			strftime(time, 256, "%c", timeinfo);

			right_len = snprintf(
				right, 256, "%s: %s",
				u.nodename, time
			);

			if (left_len <= 0 || right_len <= 0) {
				perror("please try to execute without the bar\n");
				exit(EXIT_FAILURE);
			}

			if (left_len + right_len >= term_x) {
				if (left_len > term_x || right_len > term_x) {
					// TODO:
				}
				printw("%-*s", term_x, left);
				printw("%*s",  term_x, right);
			} else {
				printw("%s%*s", left, term_x - left_len, right);
			}

			attroff(COLOR_PAIR(1) | A_BOLD);

			printw("\n");
		}


		// Not using endwin(3x), since it breaks with multiline
		// also-curses programs, such as ls(1) with the "-l" option.
		reset_shell_mode();
		refresh();

		if ((exec_pid = fork()) == 0) {
			if ((execvp(commandv[0], commandv)) == -1) {
				prerror(errno);
				exit(-1);
			}
		}
		waitpid(exec_pid, &ec, 0);
	
		// execvp'd command hasn't exit with success and we have
		// flag.Beep_on_error activated.
		if ( ec != 0 && flag.Beep_on_error ) {
			beep();
		}
		
		sleep( (uint)(interval) );
	}
}

