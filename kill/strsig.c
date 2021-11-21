/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)strsig.c	1.9 (gritter) 6/30/05
 */

#include <signal.h>
#include <stdlib.h>
#include <string.h>

static const struct sig_strlist {
	const int	sig_num;
	const char	*sig_str;
	const char	*sig_name;
} sig_strs[] = {
	{ 0,		"EXIT",		"UNKNOWN SIGNAL"		},
	{ SIGHUP,	"HUP",		"Hangup"			},
	{ SIGINT,	"INT",		"Interrupt"			},
	{ SIGQUIT,	"QUIT",		"Quit"				},
	{ SIGILL,	"ILL",		"Illegal Instruction"		},
	{ SIGTRAP,	"TRAP",		"Trace/Breakpoint Trap"		},
	{ SIGABRT,	"ABRT",		"Abort"				},
#ifdef	SIGIOT
	{ SIGIOT,	"IOT",		"Input/Output Trap"		},
#endif
#ifdef	SIGEMT
	{ SIGEMT,	"EMT",		"Emulation Trap"		},
#endif
#ifdef	SIGFPE
	{ SIGFPE,	"FPE",		"Arithmetic Exception"		},
#endif
#ifdef	SIGKILL
	{ SIGKILL,	"KILL",		"Killed"			},
#endif
#ifdef	SIGBUS
	{ SIGBUS,	"BUS",		"Bus Error"			},
#endif
#ifdef	SIGSEGV
	{ SIGSEGV,	"SEGV",		"Segmentation Fault"		},
#endif
#ifdef	SIGSYS
	{ SIGSYS,	"SYS",		"Bad System Call"		},
#endif
#ifdef	SIGPIPE
	{ SIGPIPE,	"PIPE",		"Broken Pipe"			},
#endif
#ifdef	SIGALRM
	{ SIGALRM,	"ALRM",		"Alarm Clock"			},
#endif
#ifdef	SIGTERM
	{ SIGTERM,	"TERM",		"Terminated"			},
#endif
#ifdef	SIGUSR1
	{ SIGUSR1,	"USR1",		"User Signal 1"			},
#endif
#ifdef	SIGUSR2
	{ SIGUSR2,	"USR2",		"User Signal 2"			},
#endif
#ifdef	SIGCLD
	{ SIGCLD,	"CLD",		"Child Status Changed"		},
#endif
#ifdef	SIGCHLD
	{ SIGCHLD,	"CHLD",		"Child Status Changed"		},
#endif
#ifdef	SIGPWR
	{ SIGPWR,	"PWR",		"Power-Fail/Restart"		},
#endif
#ifdef	SIGWINCH
	{ SIGWINCH,	"WINCH",	"Window Size Change"		},
#endif
#ifdef	SIGURG
	{ SIGURG,	"URG",		"Urgent Socket Condition"	},
#endif
#ifdef	SIGPOLL
	{ SIGPOLL,	"POLL",		"Pollable Event"		},
#endif
#ifdef	SIGIO
	{ SIGIO,	"IO",		"Input/Output Now Possible"	},
#endif
#ifdef	SIGSTOP
	{ SIGSTOP,	"STOP",		"Stopped (signal)"		},
#endif
#ifdef	SIGTSTP
	{ SIGTSTP,	"TSTP",		"Stopped (user)"		},
#endif
#ifdef	SIGCONT
	{ SIGCONT,	"CONT",		"Continued"			},
#endif
#ifdef	SIGTTIN
	{ SIGTTIN,	"TTIN",		"Stopped (tty input)"		},
#endif
#ifdef	SIGTTOU
	{ SIGTTOU,	"TTOU",		"Stopped (tty output)"		},
#endif
#ifdef	SIGVTALRM
	{ SIGVTALRM,	"VTALRM",	"Virtual Timer Expired"		},
#endif
#ifdef	SIGPROF
	{ SIGPROF,	"PROF",		"Profiling Timer Expired"	},
#endif
#ifdef	SIGXCPU
	{ SIGXCPU,	"XCPU",		"Cpu Limit Exceeded"		},
#endif
#ifdef	SIGXFSZ
	{ SIGXFSZ,	"XFSZ",		"File Size Limit Exceeded"	},
#endif
#ifdef	SIGWAITING
	{ SIGWAITING,	"WAITING",	"No runnable lwp"		},
#endif
#ifdef	SIGLWP
	{ SIGLWP,	"LWP",		"Inter-lwp signal"		},
#endif
#ifdef	SIGFREEZE
	{ SIGFREEZE,	"FREEZE",	"Checkpoint Freeze"		},
#endif
#ifdef	SIGTHAW
	{ SIGTHAW,	"THAW",		"Checkpoint Thaw"		},
#endif
#ifdef	SIGCANCEL
	{ SIGCANCEL,	"CANCEL",	"Thread Cancellation"		},
#endif
#ifdef	SIGLOST
	{ SIGLOST,	"LOST",		"Resource Lost"			},
#endif
#ifdef	SIGSTKFLT
	{ SIGSTKFLT,	"STKFLT",	"Stack Fault On Coprocessor"	},
#endif
#ifdef	SIGINFO
	{ SIGINFO,	"INFO",		"Status Request From Keyboard"	},
#endif
#ifdef	SIG_2_STR_WITH_RT_SIGNALS
	{ SIGRTMIN,	"RTMIN",	"First Realtime Signal"		},
	{ SIGRTMIN+1,	"RTMIN+1",	"Second Realtime Signal"	},
	{ SIGRTMIN+2,	"RTMIN+2"	"Third Realtime Signal"		},
	{ SIGRTMIN+3,	"RTMIN+3",	"Fourth Realtime Signal"	},
	{ SIGRTMAX-3,	"RTMAX-3",	"Fourth Last Realtime Signal"	},
	{ SIGRTMAX-2,	"RTMAX-2",	"Third Last Realtime Signal"	},
	{ SIGRTMAX-1,	"RTMAX-1",	"Second Last Realtime Signal"	},
	{ SIGRTMAX,	"RTMAX"	},	"Last Realtime Signal"		},
#endif	/* SIG_2_STR_WITH_RT_SIGNALS */
	{ -1,		NULL	}
};

int 
str_2_sig(const char *str, int *signum)
{
	register int	i;
	long	n;
	char	*x;

	for (i = 0; sig_strs[i].sig_str; i++)
		if (strcmp(str, sig_strs[i].sig_str) == 0)
			break;
	if (sig_strs[i].sig_str == NULL) {
		n = strtol(str, &x, 10);
		if (*x != '\0' || n < 0 || n >= i || *str == '+' || *str == '-')
			return -1;
		*signum = n;
	} else
		*signum = sig_strs[i].sig_num;
	return 0;
}

int 
sig_2_str(int signum, char *str)
{
	register int	i;

	for (i = 0; sig_strs[i].sig_str; i++)
		if (sig_strs[i].sig_num == signum)
			break;
	if (sig_strs[i].sig_str == NULL)
		return -1;
	strcpy(str, sig_strs[i].sig_str);
	return 0;
}

char *
str_signal(int signum)
{
	register int	i;

	for (i = 0; sig_strs[i].sig_name; i++)
		if (sig_strs[i].sig_num == signum)
			break;
	return (char *)sig_strs[i].sig_name;
}
