/*
 * watch.c - Keep an eye on a command output
 *
 */
/*
 * Copyright (C) 2025: Luiz Antônio Rangel (takusuman)
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strmenta.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static char *progname;

void usage(void) {
	pfmt(
		stderr, MM_NOSTD,
		"usage: %s [-n seconds] [-btx] command [args...]\n",
		progname
	);
	exit(1);
}

int main(int argc, char *argv[]) {
	progname = argv[0];
	int option = 0;
	int fBeep_on_error = 0,
	    fExec = 0,
	    fNo_title = 0,
	    c = 0, ec = 0,
	    term_x = 0;
	char **commandv;
	pid_t exec_pid;
	struct timespec interval;

	/*
	 * Default interval of 2 seconds, as other major
	 * implementations usually do.
	 */
	interval.tv_sec = 2;
	interval.tv_nsec = 0;

	/*
	 * Variables for the information header.
	 * Defining nodename from now, since it shouldn't change
	 * while we're watching the command.
	 */
	time_t now;
	struct tm *timeinfo;
	struct utsname u;
	if (uname(&u) == -1) {
		pfmt(stderr, MM_ERROR,
			"%s: failed to get system info: %s",
			progname, strerror(errno));
		exit(-1);
	}

	while ((option = getopt(argc, argv, "n:bthx")) != -1) {
		switch (option) {
		case 'n':
			if (!optarg) {
			       	break;
			}
			char arg[128];
			strncpy(arg, optarg, 128);
			arg[127] = '\0';

			char *afterpoint = strchr(arg, ',');
			if (!afterpoint)
				afterpoint = strchr(arg, '.');

			if (afterpoint) {
				*afterpoint = '\0';
				afterpoint++;
			}

			if (strlen(arg) == 0)
			       	interval.tv_sec = 0;
			else
			       	interval.tv_sec = atoi(arg);

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

			if (interval.tv_sec == 0 && interval.tv_nsec < 100000000)
				interval.tv_nsec = 100000000;

			break;
		case 'b':
			fBeep_on_error = 1;
			break;
		case 't':
			fNo_title = 1;
			break;
		case 'x':
			fExec = 1;
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
		pfmt(stderr, MM_ERROR,
			"%s: couldn't calloc(): %s",
			progname, strerror(errno));
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
	use_default_colors();
	init_pair(1, COLOR_BLACK, COLOR_WHITE);

	/*
	 * Declaring the left side of the information header, which contains
	 * "Every χ second(s): ...", outside the loop because it's immutable
	 * after we got the program we're going to run and the amount of time.
	 * Also initialize 'char left[]' with a null character for preventing
	 * it being considered an "incomplete type" by the compiler.
	 */
	int left_len = 0;
	char left[256];
	left[0] = '\0';
	if (!fNo_title) {
		left_len = snprintf(
			left, 256, "Every %lu.%02lu second(s): %s",
			interval.tv_sec, interval.tv_nsec / 10000000, argv[0]
		);
	}

	/* Create command line string for system(3). */
	char *commandl = NULL;
	if (!fExec)
		commandl = strjoin(commandv, " ");

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
				pfmt(stderr, MM_ERROR,
					"%s: please try to execute it without the information header.\n",
					progname);
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

		if (fExec) {
			if ((exec_pid = fork()) == 0) {
				if ((execvp(commandv[0], commandv)) == -1) {
					pfmt(stderr, MM_ERROR,
						"%s: couldn't exec(): %s\n",
						progname, strerror(errno));
					exit(-1);
				}
			}
			waitpid(exec_pid, &ec, 0);
		} else {
			int exec_status = system(commandl);
			if (exec_status == -1) {
				pfmt(stderr, MM_ERROR,
					"%s: couldn't system(): %s\n",
					progname, strerror(errno));
				exit(-1);
			} else {
				ec = (WIFEXITED(exec_status))
					? WEXITSTATUS(exec_status)
					: 1;
			}
		}

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

