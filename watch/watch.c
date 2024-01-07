/*
 * watch.c - Keep an eye on a command output
 *
 */
/*
 * Copyright (C) 2023, 2024: Luiz Antônio Rangel (takusuman)
 * 			     Arthur Bacci	 (arthurbacci)
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
int main(int argc, char *argv[]);
void usage(void);

int main(int argc, char *argv[]) {
	progname = argv[0];
	int option = 0;
	int fBeep_on_error = 0,
	    fNo_title = 0,
	    c = 0, ec = 0,
	    term_x = 0;
	char **commandv;
	pid_t exec_pid;
	struct timespec interval = {0};
	
	/*
	 * Default interval of 2 seconds, as other major
	 * implementations usually do.
	 */
	interval.tv_sec = 2;

	/*
	 * Variables for the information header.
	 * Defining nodename from now, since it shouldn't change
	 * while we're watching the command.
	 */
	time_t now;
	struct tm *timeinfo;
	struct utsname u;
	if (uname(&u) == -1) {
		prerror(errno);
		exit(-1);
	}

	while ((option = getopt(argc, argv, "n:bth")) != -1) {
		switch (option) {
		case 'n':
			if (!optarg) {
			       	break;
			}
			char arg[128];
			char *afterpoint = NULL;
			strncpy(arg, optarg, 128);
			arg[127] = '\0';

			size_t point = 0;
			for (; arg[point]; point++) {
				if (arg[point] == '.' || arg[point] == ',') {
					arg[point] = '\0';
					afterpoint = &arg[point + 1];
					break;
				}
			}
			
			if (strlen(arg) == 0) {
			       	interval.tv_sec = 0;
			} else {
			       	interval.tv_sec = atoi(arg);
			}

			size_t afterpointlen = afterpoint ? strlen(afterpoint) : 0;
			if (afterpointlen > 0) {
				long integer = atoi(afterpoint);

				if (afterpointlen > 9) {
					afterpoint[9] = '\0';
					afterpointlen = 9;
				}

				for (size_t i = 0; i < 9 - afterpointlen; i++)
					integer *= 10;
				
				interval.tv_nsec = integer;
			}

			if (interval.tv_sec == 0 && interval.tv_nsec < 100000000) {
				interval.tv_nsec = 100000000;
			}

			break;
		case 'b':
			fBeep_on_error = 1;
			break;
		case 't':
			fNo_title = 1;
			break;
		case 'h':
		default:
			usage();
		}
	}

	/* FIXME: Not a good practice. */
	argc -= optind;
	argv += optind;
	
	/* Missing operand. */
	if (argc < 1) {
		usage();
	}

	/*
	 * Now we just have to copy the "rest" of argv[] to a new character
	 * array allocating some space in memory with calloc(3) and then
	 * copying using a for loop.
	 */
	if ((commandv = calloc((unsigned long)(argc + 1), sizeof(char *))) == NULL) {
		/* Should I use prerror? */
		perror("couldn't callocate");
		exit(-1);
	}
	
	for (c = 0; c < argc; c++) {
		commandv[c] = argv[c];
	}

	/*
	 * Initialize curses terminal with colours to be used.
	 * Get terminal size too, we're going to need it.
	 */
	newterm(getenv("TERM"), stdout, stdin);
	start_color();
	init_pair(1, COLOR_BLACK, COLOR_WHITE);

	/*
	 * Declaring the left side of the information header, which contains
	 * "Every χ second(s): ...", outside the loop because it's immutable
	 * after we got the program we're going to run and the amount of time.
	 */
	static int left_len = 0;
	static char left[256];
	if (!fNo_title) {
		/*
		 * FIXME: When one uses "-n 0.1", it actually prints 
		 * "0.100000000" instead of 0.1 or even 0.10.
		 */
		left_len = snprintf(
			left, 256, "Every %d.%d second(s): %s",
			(int)interval.tv_sec, (int)interval.tv_nsec,
			argv[0]
		);
	}
	
	for (;;) {
		/* Clear terminal for the next cycle. */
		clear();

		/* Get the terminal maximum x-axis size. */
		term_x = getmaxx(stdscr);

		/* Get current time to be passed as a string with ctime(3). */
		time(&now);
		timeinfo = localtime(&now);

		if (!fNo_title) {
			int right_len = 0; /* "left_len = 0" declared above. */
			char right[256], time[256]; /* "left[256]" declared above. */

			attron(COLOR_PAIR(1) | A_BOLD);
		
			/*
			 * This is done because ctime returns
			 * a string with '\n'.
			 */
			strftime(time, 256, "%c", timeinfo);

			right_len = snprintf(
				right, 256, "%s: %s",
				u.nodename, time
			);

			/*
			 * I think that a case involving left_len and right_len,
			 * which contain the "Every (int).(int) second(s): (string)"
			 * and "hostname: date", respectively, is improbable, so
			 * this is just a pure formality in case of one's
			 * C library implementation having a faulty snprintf().
			 */
			if (left_len <= 0 || right_len <= 0) {
				perror("please try to execute it without the information header.\n");
				exit(255);
			}
		
			if (left_len <= term_x && right_len <= term_x) {
				/*
				 * If the sum of the text on left and right
				 * sections of the bar is larger than the
				 * terminal x axis, it shall be justified.
				 * Else, keep it as normal.
				 */
				if (left_len + right_len >= term_x) {
					printw("%-*s", term_x, left);
					printw("%*s",  term_x, right);
				} else {
					printw("%s%*s", left, (term_x - left_len), right);
				}
				printw("\n");
			}

			attroff(COLOR_PAIR(1) | A_BOLD);
		}


		/*
		 * Not using endwin(3x), since it breaks with multiline
		 * also-curses programs, such as ls(1) with the "-l" option.
		 */
		reset_shell_mode();
		refresh();

		if ((exec_pid = fork()) == 0) {
			if ((execvp(commandv[0], commandv)) == -1) {
				prerror(errno);
				exit(-1);
			}
		}
		waitpid(exec_pid, &ec, 0);
	
		/*
		 * execvp'd command hasn't exit with success and we have
		 * fBeep_on_error activated.
		 */
		if (ec != 0 && fBeep_on_error) {
			beep();
		}
		
		nanosleep(&interval, NULL);
	}
}

void usage(void) {
	pfmt(
		stderr, MM_NOSTD,
		"usage: %s [-n seconds] [-bt] command [args...]\n",
		progname
	);
	exit(1);
}
