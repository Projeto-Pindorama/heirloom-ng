/*
 * sigtable.h - signal map-like struct.
 */
/*
 * Derived from kill/strsig.c
 *
 * Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
 * Copyright (C) 2023, 2024: Luiz Antônio (takusuman)
 *
 * SPDX-Licence-Identifier: CDDL-1.0
 */

#include <signal.h>

struct sig_strlist {
	int   signum;
	char *signame;
};

static const struct sig_strlist sig_strs[] = {
	{ 0,		"EXIT" },	/* UNKNOWN SIGNAL */
	{ SIGHUP,	"HUP" }, 	/* Hangup */
	{ SIGINT,	"INT" }, 	/* Interrupt */
	{ SIGQUIT,	"QUIT" }, 	/* Quit */
	{ SIGILL,	"ILL" }, 	/* Illegal Instruction */
	{ SIGTRAP,	"TRAP" }, 	/* Trace/Breakpoint Trap */
	{ SIGABRT,	"ABRT" }, 	/* Abort */
#ifdef	SIGIOT
	{ SIGIOT,	"IOT" }, 	/* Input/Output Trap */
#endif
#ifdef	SIGEMT
	{ SIGEMT,	"EMT" }, 	/* Emulation Trap */
#endif
#ifdef	SIGFPE
	{ SIGFPE,	"FPE" }, 	/* Arithmetic Exception */
#endif
#ifdef	SIGKILL
	{ SIGKILL,	"KILL" }, 	/* Killed */
#endif
#ifdef	SIGBUS
	{ SIGBUS,	"BUS" }, 	/* Bus Error */
#endif
#ifdef	SIGSEGV
	{ SIGSEGV,	"SEGV" }, 	/* Segmentation Fault */
#endif
#ifdef	SIGSYS
	{ SIGSYS,	"SYS" }, 	/* Bad System Call */
#endif
#ifdef	SIGPIPE
	{ SIGPIPE,	"PIPE" }, 	/* Broken Pipe */
#endif
#ifdef	SIGALRM
	{ SIGALRM,	"ALRM" }, 	/* Alarm Clock */
#endif
#ifdef	SIGTERM
	{ SIGTERM,	"TERM" }, 	/* Terminated */
#endif
#ifdef	SIGUSR1
	{ SIGUSR1,	"USR1" }, 	/* User Signal 1 */
#endif
#ifdef	SIGUSR2
	{ SIGUSR2,	"USR2" }, 	/* User Signal 2 */
#endif
#ifdef	SIGCLD
	{ SIGCLD,	"CLD" }, 	/* Child Status Changed */
#endif
#ifdef	SIGCHLD
	{ SIGCHLD,	"CHLD" }, 	/* Child Status Changed */
#endif
#ifdef	SIGPWR
	{ SIGPWR,	"PWR" }, 	/* Power-Fail/Restart */
#endif
#ifdef	SIGWINCH
	{ SIGWINCH,	"WINCH" }, 	/* Window Size Change */
#endif
#ifdef	SIGURG
	{ SIGURG,	"URG" }, 	/* Urgent Socket Condition */
#endif
#ifdef	SIGPOLL
	{ SIGPOLL,	"POLL" }, 	/* Pollable Event */
#endif
#ifdef	SIGIO
	{ SIGIO,	"IO" }, 	/* Input/Output Now Possible */
#endif
#ifdef	SIGSTOP
	{ SIGSTOP,	"STOP" }, 	/* Stopped (signal) */
#endif
#ifdef	SIGTSTP
	{ SIGTSTP,	"TSTP" }, 	/* Stopped (user) */
#endif
#ifdef	SIGCONT
	{ SIGCONT,	"CONT" }, 	/* Continued */
#endif
#ifdef	SIGTTIN
	{ SIGTTIN,	"TTIN" }, 	/* Stopped (tty input) */
#endif
#ifdef	SIGTTOU
	{ SIGTTOU,	"TTOU" }, 	/* Stopped (tty output) */
#endif
#ifdef	SIGVTALRM
	{ SIGVTALRM,	"VTALRM" }, 	/* Virtual Timer Expired */
#endif
#ifdef	SIGPROF
	{ SIGPROF,	"PROF" }, 	/* Profiling Timer Expired */
#endif
#ifdef	SIGXCPU
	{ SIGXCPU,	"XCPU" }, 	/* Cpu Limit Exceeded */
#endif
#ifdef	SIGXFSZ
	{ SIGXFSZ,	"XFSZ" }, 	/* File Size Limit Exceeded */
#endif
#ifdef	SIGWAITING
	{ SIGWAITING,	"WAITING" }, 	/* No runnable lwp */
#endif
#ifdef	SIGLWP
	{ SIGLWP,	"LWP" }, 	/* Inter-lwp signal */
#endif
#ifdef	SIGFREEZE
	{ SIGFREEZE,	"FREEZE" }, 	/* Checkpoint Freeze */
#endif
#ifdef	SIGTHAW
	{ SIGTHAW,	"THAW" }, 	/* Checkpoint Thaw */
#endif
#ifdef	SIGCANCEL
	{ SIGCANCEL,	"CANCEL" }, 	/* Thread Cancellation */
#endif
#ifdef	SIGLOST
	{ SIGLOST,	"LOST" }, 	/* Resource Lost */
#endif
#ifdef	SIGSTKFLT
	{ SIGSTKFLT,	"STKFLT" }, 	/* Stack Fault On Coprocessor */
#endif
#ifdef	SIGINFO
	{ SIGINFO,	"INFO" }, 	/* Status Request From Keyboard */
#endif
#ifdef	SIG_2_STR_WITH_RT_SIGNALS
	{ SIGRTMIN,	"RTMIN" }, 	/* First Realtime Signal */
	{ SIGRTMIN+1,	"RTMIN+1" }, 	/* Second Realtime Signal */
	{ SIGRTMIN+2, 	"RTMIN+2" }, 	/* Third Realtime Signal */
	{ SIGRTMIN+3,	"RTMIN+3" }, 	/* Fourth Realtime Signal */
	{ SIGRTMAX-3,	"RTMAX-3" }, 	/* Fourth Last Realtime Signal */
	{ SIGRTMAX-2,	"RTMAX-2" }, 	/* Third Last Realtime Signal */
	{ SIGRTMAX-1,	"RTMAX-1" }, 	/* Second Last Realtime Signal */
	{ SIGRTMAX,	"RTMAX"	 }, 	/* Last Realtime Signal */
#endif	/* SIG_2_STR_WITH_RT_SIGNALS */
	
	/* If this is removed bad things happen dont remove this */
	{ -1,		NULL	}
};
