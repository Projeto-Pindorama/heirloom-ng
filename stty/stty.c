/*
 * stty - set the options for a terminal
 *
 * Gunnar Ritter, Freiburg i. Br., Germany, May 2003.
 */
/*
 * Copyright (c) 2003 Gunnar Ritter
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute
 * it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 */

#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 4 || __GNUC__ >= 4
#define	USED	__attribute__ ((used))
#elif defined __GNUC__
#define	USED	__attribute__ ((unused))
#else
#define	USED
#endif
#ifndef	UCB
static const char sccsid[] USED = "@(#)stty.sl	1.23 (gritter) 1/22/06";
#else	/* UCB */
static const char sccsid[] USED = "@(#)/usr/ucb/stty.sl	1.23 (gritter) 1/22/06";
#endif	/* UCB */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <ctype.h>
#include <locale.h>
#include <pathconf.h>
#ifndef	TIOCGWINSZ
#include <sys/ioctl.h>
#endif

#ifndef	VSWTCH
#ifdef	VSWTC
#define	VSWTCH	VSWTC
#endif
#endif

#ifdef	TABDLY
static void	tabs(int);
#endif
static void	evenp(int);
static void	oddp(int);
static void	spacep(int);
static void	markp(int);
static void	raw(int);
static void	cooked(int);
static void	nl(int);
static void	sane(int);
#ifdef	OFDEL
static void	fill(int);
#endif
#ifdef	XCASE
static void	lcase(int);
#endif
static void	ek(int);
#ifdef	TABDLY
static void	tty33(int);
static void	tty37(int);
static void	vt05(int);
static void	tn300(int);
static void	ti700(int);
static void	tek(int);
#endif
static void	rows(int);
static void	columns(int);
static void	ypixels(int);
static void	xpixels(int);
static void	vmin(int);
static void	vtime(int);
static void	line(int);
#ifdef	UCB
static void	litout(int);
static void	pass8(int);
static void	crt(int);
static void	dec(int);
#endif	/* UCB */

static const struct	mode {
	const char	*m_name;	/* name of mode */
	void	(*m_func)(int);		/* handler function */
	long	m_val;			/* flag value */
	long	m_dfl;			/* default (not sane!) value */
	int	m_flg;			/* print flags:
					     01 print regardless of difference
					     02 print only if -a is not set
					     04 print only if -a is set
					    010 print under all circumstances
					    020 print only if equal
					    040 ignore for printing
					   0100 ignore for setting
					   0200 use m_dfl as mask
					   0400 print only if equal even if -a
					  01000 negate for setting
					*/
	enum {
		M_SEPAR,		/* separator */
		M_NSEPAR,		/* new separator */
		M_IFLAG,		/* in c_iflag */
		M_OFLAG,		/* in c_oflag */
		M_CFLAG,		/* in c_cflag */
		M_PCFLAG,		/* in c_cflag, but printed w/o -a */
		M_LFLAG,		/* in c_lflag */
		M_CC,			/* in c_cc */
		M_FUNCT,		/* handled via function */
		M_INVAL			/* invalid */
	}	m_type;
} modes[] = {
	{ "oddp",	0, PARENB|PARODD,PARENB|PARODD,0122,	M_PCFLAG },
	{ "evenp",	0, PARENB|PARODD,PARENB,	0122,	M_PCFLAG },
	{ "parity",	0, PARENB,	0,	0122,	M_PCFLAG },
	{ "cstopb",	0,	CSTOPB,	0,	02,	M_PCFLAG },
	{ "hupcl",	0,	HUPCL,	0,	02,	M_PCFLAG },
	{ "cread",	0,	CREAD,	CREAD,	02,	M_PCFLAG },
	{ "clocal",	0,	CLOCAL,	0,	02,	M_PCFLAG },
	{ "intr",	0,	VINTR,	'\177',	0,	M_CC },
	{ "quit",	0,	VQUIT,	'\34',	0,	M_CC },
	{ "erase",	0,	VERASE,	'#',	0,	M_CC },
	{ "kill",	0,	VKILL,	'@',	0,	M_CC },
	{ "\n",		0,	0,	0,	04,	M_NSEPAR },
	{ "eof",	0,	VEOF,	'\4',	0,	M_CC },
	{ "eol",	0,	VEOL,	'\0',	0,	M_CC },
	{ "eol2",	0,	VEOL2,	'\0',	0,	M_CC },
#ifdef	VSWTCH
	{ "swtch",	0,	VSWTCH,	'\32',	0,	M_CC },
#endif
	{ "\n",		0,	0,	0,	04,	M_NSEPAR },
	{ "start",	0,	VSTART,	'\21',	0,	M_CC },
	{ "stop",	0,	VSTOP,	'\23',	0,	M_CC },
	{ "susp",	0,	VSUSP,	'\32',	0,	M_CC },
#ifdef	VDSUSP
	{ "dsusp",	0,	VDSUSP,	'\31',	0,	M_CC },
#else
	{ "dsusp",	0,	-1,	'\0',	0,	M_CC },
#endif
	{ "\n",		0,	0,	0,	04,	M_NSEPAR },
#ifdef	VREPRINT
	{ "rprnt",	0,	VREPRINT,'\22',	0,	M_CC },
#else
	{ "rprnt",	0,	-1,	'\0',	0,	M_CC },
#endif
#ifdef	VDISCARD
	{ "flush",	0,	VDISCARD,'\17',	0,	M_CC },
#else
	{ "flush",	0,	-1,	'\0',	0,	M_CC },
#endif
#ifdef	VWERASE
	{ "werase",	0,	VWERASE,'\27',	0,	M_CC },
#else
	{ "werase",	0,	-1,	'\0',	0,	M_CC },
#endif
	{ "lnext",	0,	VLNEXT,	'\26',	0,	M_CC },
	{ "\n",		0,	0,	0,	010,	M_SEPAR },
	{ "parenb",	0,	PARENB,	0,	04,	M_CFLAG },
	{ "parodd",	0,	PARODD,	0,	04,	M_CFLAG },
	{ "cs5",	0,	CS5,	CSIZE,	0604,	M_CFLAG },
	{ "cs6",	0,	CS6,	CSIZE,	0604,	M_CFLAG },
	{ "cs7",	0,	CS7,	CSIZE,	0604,	M_CFLAG },
	{ "cs8",	0,	CS8,	CSIZE,	0604,	M_CFLAG },
	{ "cstopb",	0,	CSTOPB,	0,	04,	M_CFLAG },
	{ "hupcl",	0,	HUPCL,	0,	04,	M_CFLAG },
	{ "hup",	0,	HUPCL,	0,	040,	M_CFLAG },
	{ "cread",	0,	CREAD,	CREAD,	04,	M_CFLAG },
	{ "clocal",	0,	CLOCAL,	0,	04,	M_CFLAG },
#ifdef	LOBLK
	{ "loblk",	0,	LOBLK,	0,	04,	M_CFLAG },
#else
	{ "loblk",	0,	0,	0,	04,	M_INVAL },
#endif
#ifdef	PAREXT
	{ "parext",	0,	PAREXT,	0,	04,	M_CFLAG },
#else
	{ "parext",	0,	0,	0,	04,	M_INVAL },
#endif
	{ "\n",		0,	0,	0,	04,	M_SEPAR },
	{ "ignbrk",	0,	IGNBRK,	0,	0,	M_IFLAG },
	{ "brkint",	0,	BRKINT,	0,	04,	M_IFLAG },
#ifndef	UCB
	{ "brkint",	0,IGNBRK|BRKINT,BRKINT,	0122,	M_IFLAG },
#else	/* UCB */
	{ "brkint",	0,	0,	BRKINT,	0122,	M_IFLAG },
#endif	/* UCB */
	{ "ignpar",	0,	IGNPAR,	IGNPAR,	04,	M_IFLAG },
	{ "inpck",	0,	INPCK,	INPCK,	0102,	M_IFLAG },
	{ "ignpar",	0,INPCK|IGNPAR,	IGNPAR,	0102,	M_IFLAG },
	{ "parmrk",	0,	PARMRK,	0,	0,	M_IFLAG },
	{ "inpck",	0,	INPCK,	INPCK,	04,	M_IFLAG },
	{ "istrip",	0,	ISTRIP,	ISTRIP,	0,	M_IFLAG },
	{ "inlcr",	0,	INLCR,	0,	0,	M_IFLAG },
	{ "igncr",	0,	IGNCR,	0,	0,	M_IFLAG },
#ifndef	UCB
	{ "icrnl",	0,	ICRNL,	0,	0,	M_IFLAG },
#else	/* UCB */
	{ "icrnl",	0,	ICRNL,	ICRNL,	0,	M_IFLAG },
#endif	/* UCB */
#ifdef	IUCLC
	{ "iuclc",	0,	IUCLC,	0,	0,	M_IFLAG },
#endif
	{ "\n",		0,	0,	0,	04,	M_SEPAR },
	{ "ixon",	0,	IXON,	IXON,	04,	M_IFLAG },
#ifndef	UCB
	{ "ixany",	0,	IXANY,	IXANY,	04,	M_IFLAG },
#else	/* UCB */
	{ "ixany",	0,	IXANY,	0,	0,	M_IFLAG },
	{ "decctlq",	0,	IXANY,	0,	01040,	M_IFLAG },
#endif	/* UCB */
	{ "ixon",	0,	0,	IXON,	0302,	M_IFLAG },
	{ "ixoff",	0,	IXOFF,	0,	0,	M_IFLAG },
#ifdef	UCB
	{ "tandem",	0,	IXOFF,	0,	040,	M_IFLAG },
#endif	/* UCB */
	{ "imaxbel",	0,	IMAXBEL,0,	0,	M_IFLAG },
#ifdef	IUTF8
	{ "iutf8",	0,	IUTF8,	0,	0,	M_IFLAG },
#endif
	{ "\n",		0,	0,	0,	04,	M_SEPAR },
	{ "isig",	0,	ISIG,	ISIG,	04,	M_LFLAG },
	{ "icanon",	0,	ICANON,	ICANON,	04,	M_LFLAG },
	{ "cbreak",	0,	ICANON,	0,	01040,	M_LFLAG },
#ifdef	XCASE
	{ "xcase",	0,	XCASE,	0,	04,	M_LFLAG },
#endif
	{ "opost",	0,	OPOST,	OPOST,	0102,	M_OFLAG },
#ifdef	OLCUC
	{ "olcuc",	0,	OLCUC,	0,	0102,	M_OFLAG },
#endif
#ifndef	UCB
	{ "onlcr",	0,	ONLCR,	0,	0102,	M_OFLAG },
#else	/* UCB */
	{ "onlcr",	0,	ONLCR,	ONLCR,	0102,	M_OFLAG },
#endif	/* UCB */
	{ "ocrnl",	0,	OCRNL,	0,	0102,	M_OFLAG },
	{ "onocr",	0,	ONOCR,	0,	0102,	M_OFLAG },
	{ "onlret",	0,	ONLRET,	0,	0102,	M_OFLAG },
#if defined (OFILL) && defined (OFDEL)
	{ "nul-fill",	0,	OFILL,OFILL|OFDEL,0202,	M_OFLAG },
	{ "del-fill",	0,OFILL|OFDEL,OFILL|OFDEL,0202,	M_OFLAG },
#endif
#ifdef	TAB1
	{ "tab1",	0,	TAB1,	TABDLY,	0302,	M_OFLAG },
#endif
#ifdef	TAB2
	{ "tab2",	0,	TAB2,	TABDLY,	0302,	M_OFLAG },
#endif
#ifdef	TAB3
	{ "tab3",	0,	TAB3,	TABDLY,	0302,	M_OFLAG },
#endif
	{ "\n",		0,	0,	0,	02,	M_SEPAR },
	{ "isig",	0,	ISIG,	ISIG,	0102,	M_LFLAG },
	{ "icanon",	0,	ICANON,	ICANON,	0102,	M_LFLAG },
#ifdef	XCASE
	{ "xcase",	0,	XCASE,	0,	0102,	M_LFLAG },
#endif
#ifndef	UCB
	{ "echo",	0,	ECHO,	ECHO,	01,	M_LFLAG },
	{ "echoe",	0,	ECHOE,	ECHOE,	01,	M_LFLAG },
	{ "echok",	0,	ECHOK,	ECHOK,	01,	M_LFLAG },
#else	/* UCB */
	{ "echo",	0,	ECHO,	ECHO,	0,	M_LFLAG },
	{ "echoe",	0,	ECHOE,	ECHOE,	04,	M_LFLAG },
	{ "crterase",	0,	ECHOE,	0,	040,	M_LFLAG },
	{ "echok",	0,	ECHOK,	ECHOK,	0,	M_LFLAG },
	{ "lfkc",	0,	ECHOK,	0,	040,	M_LFLAG },
	{ "echoe",	0,ECHOE|ECHOKE,	ECHOE,	0122,	M_LFLAG },
	{ "-echoke",	0,ECHOE|ECHOKE,	ECHOE,	0122,	M_LFLAG },
	{ "echoprt",	0,ECHOE|ECHOPRT,0,	0122,	M_LFLAG },
	{ "crt",	0,ECHOE|ECHOKE,ECHOE|ECHOKE,0302,M_LFLAG },
#endif	/* UCB */
	{ "lfkc",	0,	ECHOK,	0,	040,	M_LFLAG },
	{ "echonl",	0,	ECHONL,	0,	0,	M_LFLAG },
	{ "noflsh",	0,	NOFLSH,	0,	0,	M_LFLAG },
	{ "\n",		0,	0,	0,	04,	M_NSEPAR },
	{ "tostop",	0,	TOSTOP,	0,	0,	M_LFLAG },
#ifndef	UCB
	{ "echoctl",	0,	ECHOCTL,0,	0,	M_LFLAG },
#else	/* UCB */
	{ "echoctl",	0,	ECHOCTL,ECHOCTL,0,	M_LFLAG },
	{ "ctlecho",	0,	ECHOCTL,0,	040,	M_LFLAG },
	{ "prterase",	0,	ECHOPRT,0,	040,	M_LFLAG },
#endif	/* UCB */
	{ "echoprt",	0,	ECHOPRT,0,	04,	M_LFLAG },
#ifndef	UCB
	{ "echoke",	0,	ECHOKE,	0,	0,	M_LFLAG },
#else	/* UCB */
	{ "echoke",	0,	ECHOKE,	0,	04,	M_LFLAG },
	{ "crtkill",	0,	ECHOKE,	0,	040,	M_LFLAG },
#endif	/* UCB */
	{ "defecho",	0,	0,	0,	0,	M_INVAL },
	{ "flusho",	0,	FLUSHO,	0,	0,	M_LFLAG },
	{ "pendin",	0,	PENDIN,	0,	0,	M_LFLAG },
	{ "iexten",	0,	IEXTEN,	0,	0,	M_LFLAG },
	{ "\n",		0,	0,	0,	04,	M_SEPAR },
	{ "opost",	0,	OPOST,	OPOST,	04,	M_OFLAG },
#ifdef	OLCUC
	{ "olcuc",	0,	OLCUC,	0,	04,	M_OFLAG },
#endif
	{ "onlcr",	0,	ONLCR,	0,	04,	M_OFLAG },
	{ "ocrnl",	0,	OCRNL,	0,	04,	M_OFLAG },
	{ "onocr",	0,	ONOCR,	0,	04,	M_OFLAG },
	{ "onlret",	0,	ONLRET,	0,	04,	M_OFLAG },
#ifdef	OFILL
	{ "ofill",	0,	OFILL,	0,	04,	M_OFLAG },
#endif
#ifdef	OFDEL
	{ "ofdel",	0,	OFDEL,	0,	04,	M_OFLAG },
#endif
#ifdef	TAB1
	{ "tab1",	0,	TAB1,	TABDLY,	0704,	M_OFLAG },
#endif
#ifdef	TAB2
	{ "tab2",	0,	TAB2,	TABDLY,	0704,	M_OFLAG },
#endif
#ifdef	TAB3
	{ "tab3",	0,	TAB3,	TABDLY,	0704,	M_OFLAG },
#endif
	{ "\n",		0,	0,	0,	04,	M_SEPAR },
#ifdef	NL0
	{ "nl0",	0,	NL0,	NLDLY,	0240,	M_OFLAG },
#endif
#ifdef	NL1
	{ "nl1",	0,	NL1,	NLDLY,	0240,	M_OFLAG },
#endif
#ifdef	CR0
	{ "cr0",	0,	CR0,	CRDLY,	0240,	M_OFLAG },
#endif
#ifdef	CR1
	{ "cr1",	0,	CR1,	CRDLY,	0240,	M_OFLAG },
#endif
#ifdef	CR2
	{ "cr2",	0,	CR2,	CRDLY,	0240,	M_OFLAG },
#endif
#ifdef	CR3
	{ "cr3",	0,	CR3,	CRDLY,	0240,	M_OFLAG },
#endif
#ifdef	TAB0
	{ "tab0",	0,	TAB0,	TABDLY,	0240,	M_OFLAG },
#endif
#ifdef	TAB1
	{ "tab1",	0,	TAB1,	TABDLY,	0240,	M_OFLAG },
#endif
#ifdef	TAB2
	{ "tab2",	0,	TAB2,	TABDLY,	0240,	M_OFLAG },
#endif
#ifdef	TAB3
	{ "tab3",	0,	TAB3,	TABDLY,	0240,	M_OFLAG },
#endif
#ifdef	TABDLY
	{ "tabs",	tabs,	0,	0,	0240,	M_FUNCT },
#endif
#ifdef	BS0
	{ "bs0",	0,	BS0,	BSDLY,	0240,	M_OFLAG },
#endif
#ifdef	BS1
	{ "bs1",	0,	BS1,	BSDLY,	0240,	M_OFLAG },
#endif
#ifdef	FF0
	{ "ff0",	0,	FF0,	FFDLY,	0240,	M_OFLAG },
#endif
#ifdef	FF1
	{ "ff1",	0,	FF1,	FFDLY,	0240,	M_OFLAG },
#endif
#ifdef	VT0
	{ "vt0",	0,	VT0,	VTDLY,	0240,	M_OFLAG },
#endif
#ifdef	VT1
	{ "vt1",	0,	VT1,	VTDLY,	0240,	M_OFLAG },
#endif
#ifdef	CRTSCTS
	{ "ctsxon",	0,	CRTSCTS,0,	040,	M_OFLAG },
#endif	/* CRTSCTS */
	{ "evenp",	evenp,	0,	0,	040,	M_FUNCT },
	{ "parity",	evenp,	0,	0,	040,	M_FUNCT },
	{ "oddp",	oddp,	0,	0,	040,	M_FUNCT },
	{ "spacep",	spacep,	0,	0,	040,	M_FUNCT },
	{ "markp",	markp,	0,	0,	040,	M_FUNCT },
	{ "raw",	raw,	0,	0,	040,	M_FUNCT },
	{ "cooked",	cooked,	0,	0,	040,	M_FUNCT },
	{ "nl",		nl,	0,	0,	040,	M_FUNCT },
#ifdef	XCASE
	{ "lcase",	lcase,	0,	0,	040,	M_FUNCT },
	{ "LCASE",	lcase,	0,	0,	040,	M_FUNCT },
#endif
	{ "ek",		ek,	0,	0,	040,	M_FUNCT },
	{ "sane",	sane,	0,	0,	040,	M_FUNCT },
#ifdef	OFDEL
	{ "fill",	fill,	0,	0,	040,	M_FUNCT },
#endif
#ifdef	TABDLY
	{ "tty33",	tty33,	0,	0,	040,	M_FUNCT },
	{ "tty37",	tty37,	0,	0,	040,	M_FUNCT },
	{ "vt05",	vt05,	0,	0,	040,	M_FUNCT },
	{ "tn300",	tn300,	0,	0,	040,	M_FUNCT },
	{ "ti700",	ti700,	0,	0,	040,	M_FUNCT },
	{ "tek",	tek,	0,	0,	040,	M_FUNCT },
#endif
	{ "rows",	rows,	0,	0,	040,	M_FUNCT },
	{ "columns",	columns,0,	0,	040,	M_FUNCT },
#ifdef	UCB
	{ "cols",	columns,0,	0,	040,	M_FUNCT },
#endif	/* UCB */
	{ "ypixels",	ypixels,0,	0,	040,	M_FUNCT },
	{ "xpixels",	xpixels,0,	0,	040,	M_FUNCT },
	{ "min",	vmin,	0,	0,	040,	M_FUNCT },
	{ "time",	vtime,	0,	0,	040,	M_FUNCT },
	{ "line",	line,	0,	0,	040,	M_FUNCT },
#ifdef	UCB
	{ "litout",	litout,	0,	0,	040,	M_FUNCT },
	{ "pass8",	pass8,	0,	0,	040,	M_FUNCT },
	{ "crt",	crt,	0,	0,	040,	M_FUNCT },
	{ "dec",	dec,	0,	0,	040,	M_FUNCT },
#endif	/* UCB */
	{ 0,		0,	0,	0,	0,	M_INVAL }
};

static const struct {
	const char	*s_str;
	speed_t	s_val;
} speeds[] = {
	{ "0",		B0 },
	{ "50",		B50 },
	{ "75",		B75 },
	{ "110",	B110 },
	{ "134",	B134 },
	{ "134.5",	B134 },
	{ "150",	B150 },
	{ "200",	B200 },
	{ "300",	B300 },
	{ "600",	B600 },
	{ "1200",	B1200 },
	{ "1800",	B1800 },
	{ "2400",	B2400 },
	{ "4800",	B4800 },
	{ "9600",	B9600 },
	{ "19200",	B19200 },
	{ "19.2",	B19200 },
	{ "38400",	B38400 },
	{ "38.4",	B38400 },
#ifdef	B57600
	{ "57600",	B57600 },
#endif	/* B57600 */
#ifdef	B115200
	{ "115200",	B115200 },
#endif	/* B115200 */
#ifdef	B230400
	{ "230400",	B230400 },
#endif	/* B230400 */
#ifdef	B460800
	{ "460800",	B460800 },
#endif	/* B460800 */
#ifdef	B500000
	{ "500000",	B500000 },
#endif	/* B500000 */
#ifdef	B576000
	{ "576000",	B576000 },
#endif	/* B576000 */
#ifdef	B921600
	{ "921600",	B921600 },
#endif	/* B921600 */
#ifdef	B1000000
	{ "1000000",	B1000000 },
#endif	/* B1000000 */
#ifdef	B1152000
	{ "1152000",	B1152000 },
#endif	/* B1152000 */
#ifdef	B1500000
	{ "1500000",	B1500000 },
#endif	/* B1500000 */
#ifdef	B2000000
	{ "2000000",	B2000000 },
#endif	/* B2000000 */
#ifdef	B2500000
	{ "2500000",	B2500000 },
#endif	/* B2500000 */
#ifdef	B3000000
	{ "3000000",	B3000000 },
#endif	/* B3000000 */
#ifdef	B3500000
	{ "3500000",	B3500000 },
#endif	/* B3500000 */
#ifdef	B4000000
	{ "4000000",	B4000000 },
#endif	/* B4000000 */
#ifdef	EXTA
	{ "exta",	EXTA },
#endif	/* EXTA */
#ifdef	EXTB
	{ "extb",	EXTB },
#endif	/* EXTB */
	{ 0,		0 }
};

static const char	*progname;
static const char	**args;
static struct termios	ts;
static struct winsize	ws;
static int		wschange;	/* ws was changed */
static long		vdis;		/* VDISABLE character */
extern int		sysv3;

static void	usage(void);
static void	getattr(int);
static void	list(int, int);
static int	listmode(tcflag_t, struct mode, int, int);
static int	listchar(cc_t *, struct mode, int, int);
static const char	*baudrate(speed_t c);
static void	set(void);
static void	setmod(tcflag_t *, struct mode, int);
static void	setchr(cc_t *, struct mode);
static void	inval(void);
static void	glist(void);
static void	gset(void);
#ifdef	UCB
static void	hlist(int);
static void	speed(void);
static void	size(void);
#endif	/* UCB */

#ifndef	UCB
#define	STTYFD	0
#else	/* UCB */
#define	STTYFD	1
#endif	/* UCB */

int
main(int argc, char **argv)
{
	int	dds;

	progname = basename(argv[0]);
	setlocale(LC_CTYPE, "");
#ifndef	UCB
	if (getenv("SYSV3") != NULL)
		sysv3 = 1;
	getattr(STTYFD);
#endif	/* !UCB */
	if (argc >= 2 && strcmp(argv[1], "--") == 0) {
		argc--, argv++;
		dds = 1;
	} else
		dds = 0;
	args = argc ? (const char **)&argv[1] : (const char **)&argv[0];
	if(!dds && argc == 2 && argv[1][0] == '-' && argv[1][1] &&
			argv[1][2] == '\0') {
		switch (argv[1][1]) {
		case 'a':
#ifdef	UCB
		getattr(STTYFD);
#endif	/* UCB */
			list(1, 0);
			break;
		case 'g':
#ifdef	UCB
		getattr(-1);
#endif	/* UCB */
			glist();
			break;
#ifdef	UCB
		case 'h':
		getattr(STTYFD);
			hlist(1);
			break;
#endif	/* UCB */
		default:
			usage();
		}
	} else if (argc == 1) {
#ifdef	UCB
		getattr(STTYFD);
#endif	/* UCB */
		list(0, 0);
#ifdef	UCB
	} else if (argc == 2 && strcmp(argv[1], "all") == 0) {
		getattr(STTYFD);
		hlist(0);
	} else if (argc == 2 && strcmp(argv[1], "everything") == 0) {
		getattr(STTYFD);
		hlist(1);
	} else if (argc == 2 && strcmp(argv[1], "speed") == 0) {
		getattr(-1);
		speed();
	} else if (argc == 2 && strcmp(argv[1], "size") == 0) {
		getattr(-1);
		size();
#endif	/* UCB */
	} else {
#ifdef	UCB
		getattr(STTYFD);
#endif	/* UCB */
		set();
		if (tcsetattr(STTYFD, TCSADRAIN, &ts) < 0 ||
				wschange && ioctl(STTYFD, TIOCSWINSZ, &ws) < 0)
			perror(progname);
	}
	return 0;
}

static void
usage(void)
{
	fprintf(stderr, "usage: %s [-ag] [modes]\n", progname);
	exit(2);
}

static void
getattr(int fd)
{
#ifdef	UCB
	const char	devtty[] = "/dev/tty";

	if (fd < 0 && (fd = open(devtty, O_RDONLY)) < 0) {
		fprintf(stderr, "%s: Cannot open %s: %s\n",
				progname, devtty, strerror(errno));
		exit(2);
	}
#endif	/* UCB */
	if (tcgetattr(fd, &ts) < 0) {
		perror(progname);
		exit(2);
	}
	if (ioctl(fd, TIOCGWINSZ, &ws) < 0) {
		ws.ws_row = 0;
		ws.ws_col = 0;
		ws.ws_xpixel = 0;
		ws.ws_ypixel = 0;
	}
#if !defined (__FreeBSD__) && !defined (__DragonFly__) && !defined (__APPLE__)
	vdis = fpathconf(fd, _PC_VDISABLE) & 0377;
#else
	vdis = '\377' & 0377;
#endif
}

static void
list(int aflag, int hflag)
{
	int	i, d = 0;
	speed_t	is, os;

	is = cfgetispeed(&ts);
	os = cfgetospeed(&ts);
	if (is == os)
		printf("speed %s baud;", baudrate(is));
	else
		printf("ispeed %s baud; ospeed %s baud;",
				baudrate(is), baudrate(os));
	if (aflag == 0) {
		for (i = 0; modes[i].m_name; i++) {
			if (modes[i].m_type == M_PCFLAG)
				d += listmode(ts.c_cflag, modes[i], aflag, 1);
		}
		d = 0;
	}
	if (sysv3 && aflag == 0) {
		putchar('\n');
	} else {
		putchar(sysv3 ? ' ' : '\n');
		printf("rows = %d%s columns = %d; "
				"ypixels = %d%s xpixels = %d%s\n",
			(int)ws.ws_row,
			aflag&&hflag ? "" : ";",
			(int)ws.ws_col,
			(int)ws.ws_ypixel,
			aflag&&hflag ? "" : ";",
			(int)ws.ws_xpixel,
			aflag&&hflag ? "" : ";");
	}
	if ((ts.c_lflag&ICANON) == 0)
		printf("min = %d; time = %d;\n",
				(int)ts.c_cc[VMIN], (int)ts.c_cc[VTIME]);
	for (i = 0; modes[i].m_name; i++) {
		if (modes[i].m_flg&040)
			continue;
		switch (modes[i].m_type) {
		case M_NSEPAR:
			if (sysv3)
				break;
		case M_SEPAR:
			if (d && (modes[i].m_flg&8 ||
					(modes[i].m_flg&(aflag?2:4)) == 0)) {
				fputs(modes[i].m_name, stdout);
				d = 0;
			}
			break;
		case M_IFLAG:
			d += listmode(ts.c_iflag, modes[i], aflag, d);
			break;
		case M_OFLAG:
			d += listmode(ts.c_oflag, modes[i], aflag, d);
			break;
		case M_CFLAG:
			d += listmode(ts.c_cflag, modes[i], aflag, d);
			break;
		case M_LFLAG:
			d += listmode(ts.c_lflag, modes[i], aflag, d);
			break;
		case M_CC:
			if (hflag == 0)
				d += listchar(ts.c_cc, modes[i], aflag, d);
			break;
		}
		if (d >= 72 && aflag == 0) {
			putchar('\n');
			d = 0;
		}
	}
	if (d && aflag == 0)
		putchar('\n');
}

static int
listmode(tcflag_t flag, struct mode m, int aflag, int space)
{
	int	n = 0;

	if (m.m_flg&010 || (m.m_flg & (aflag?2:4)) == 0 &&
			(m.m_flg&0200 ? (flag&m.m_dfl) == m.m_val :
			 m.m_flg&020 ? (flag&m.m_val) == m.m_dfl :
				(flag&m.m_val) != m.m_dfl)
			| m.m_flg&1 |
			(aflag != 0) ^ ((m.m_flg&(aflag?0400:0)) != 0)) {
		if (space) {
			putchar(' ');
			n++;
		}
		if ((flag&m.m_val) == 0) {
			putchar('-');
			n++;
		}
		n += printf("%s", m.m_name);
	}
	return n;
}

static int
listchar(cc_t *cc, struct mode m, int aflag, int space)
{
	int	n = 0;
	char	c = m.m_val >= 0 ? cc[m.m_val] : vdis;

	if (m.m_flg&8 || (m.m_flg & (aflag?2:4)) == 0 &&
			c != (m.m_dfl?m.m_dfl:vdis) | m.m_flg&1 |
			(aflag != 0) ^ ((m.m_flg&(aflag?0400:0)) != 0)) {
		if (space) {
			putchar(' ');
			n++;
		}
		n += printf("%s ", m.m_name);
		if ((c&0377) == vdis)
			n += printf(sysv3 ? "<undef>" : "= <undef>");
		else {
			printf("= ");
			if (c & 0200) {
				c &= 0177;
				putchar('-');
				n++;
			}
			if ((c&037) == c)
				n += printf("^%c", c | (sysv3 ? 0100 : 0140));
			else if (c == '\177')
				n += printf("DEL");
			else {
				putchar(c & 0377);
				n++;
			}
		}
		putchar(';');
		n++;
	}
	return n;
}

static const char *
baudrate(speed_t c)
{
	int	i;

	for (i = 0; speeds[i].s_str; i++)
		if (speeds[i].s_val == c)
			return speeds[i].s_str;
	return "-1";
}

static void
set(void)
{
	int	i, gotcha, not, sspeed = 0;
	speed_t	ispeed0, ospeed0, ispeed1, ospeed1;
	const char	*ap;
	struct termios	tc;

	ispeed0 = ispeed1 = cfgetispeed(&ts);
	ospeed0 = ospeed1 = cfgetospeed(&ts);
	while (*args) {
		for (i = 0; speeds[i].s_str; i++)
			if (strcmp(speeds[i].s_str, *args) == 0) {
				ispeed1 = ospeed1 = speeds[i].s_val;
				sspeed |= 3;
				goto next;
			}
		gotcha = 0;
		if (**args == '-') {
			not = 1;
			ap = &args[0][1];
		} else {
			not = 0;
			ap = *args;
		}
		for (i = 0; modes[i].m_name; i++) {
			if (modes[i].m_type == M_SEPAR || modes[i].m_flg&0100)
				continue;
			if (strcmp(modes[i].m_name, ap) == 0) {
				gotcha++;
				switch (modes[i].m_type) {
				case M_IFLAG:
					setmod(&ts.c_iflag, modes[i], not);
					break;
				case M_OFLAG:
					setmod(&ts.c_oflag, modes[i], not);
					break;
				case M_CFLAG:
				case M_PCFLAG:
					setmod(&ts.c_cflag, modes[i], not);
					break;
				case M_LFLAG:
					setmod(&ts.c_lflag, modes[i], not);
					break;
				case M_CC:
					if (not)
						inval();
					setchr(ts.c_cc, modes[i]);
					break;
				case M_FUNCT:
					modes[i].m_func(not);
					break;
				}
			}
		}
		if (gotcha)
			goto next;
		if (strcmp(*args, "ispeed") == 0) {
			if (*++args == NULL)
				break;
			if (atol(*args) == 0) {
				ispeed1 = ospeed1;
				sspeed |= 1;
				goto next;
			} else for (i = 0; speeds[i].s_str; i++)
				if (strcmp(speeds[i].s_str, *args) == 0) {
					ispeed1 = speeds[i].s_val;
					sspeed |= 1;
					goto next;
				}
			inval();
		}
		if (strcmp(*args, "ospeed") == 0) {
			if (*++args == NULL)
				break;
			for (i = 0; speeds[i].s_str; i++)
				if (strcmp(speeds[i].s_str, *args) == 0) {
					ospeed1 = speeds[i].s_val;
					sspeed |= 2;
					goto next;
				}
			inval();
		}
		gset();
	next:	args++;
	}
	if (sspeed) {
		if (sspeed == 3 && ispeed1 != ospeed1 && ospeed1 != B0) {
			tc = ts;
			cfsetispeed(&tc, ispeed1);
			if (cfgetospeed(&tc) == cfgetospeed(&ts)) {
				tc = ts;
				cfsetospeed(&tc, ospeed1);
				if (cfgetispeed(&tc) == cfgetispeed(&ts)) {
					cfsetispeed(&ts, ispeed1);
					cfsetospeed(&ts, ospeed1);
				}
			}
		} else {
			if (ispeed0 != ispeed1)
				cfsetispeed(&ts, ispeed1);
			if (ospeed0 != ospeed1)
				cfsetospeed(&ts, ospeed1);
		}
	}
}

static void
setmod(tcflag_t *t, struct mode m, int not)
{
	if (m.m_flg&0200) {
		if (not)
			inval();
		*t = *t&~(tcflag_t)m.m_dfl | m.m_val;
	} else {
		if (not ^ (m.m_flg&01000) != 0)
			*t &= ~(tcflag_t)m.m_val;
		else
			*t |= m.m_val;
	}
}

static void
setchr(cc_t *cc, struct mode m)
{
	if (args[1] == NULL)
		return;
	args++;
	if (m.m_val < 0)
		return;
	if (**args == '^') {
		if (args[0][1] == '-')
			cc[m.m_val] = vdis;
		else if (args[0][1] == '?')
			cc[m.m_val] = '\177';
		else
			cc[m.m_val] = args[0][1] & 037;
	} else if (strcmp(*args, "undef") == 0)
		cc[m.m_val] = vdis;
	else
		cc[m.m_val] = **args;
}

static void
inval(void)
{
	fprintf(stderr, "unknown mode: %s\n", *args);
	exit(2);
}

#ifdef	TABDLY
static void
tabs(int not)
{
	ts.c_oflag &= ~(tcflag_t)TABDLY;
	ts.c_oflag |= not ? TAB3 : TAB0;
}
#endif

static void
evenp(int not)
{
	if (not) {
		ts.c_cflag &= ~(tcflag_t)PARENB;
		ts.c_cflag = ts.c_cflag&~(tcflag_t)CSIZE | CS8;
	} else {
		ts.c_cflag |= PARENB;
		ts.c_cflag = ts.c_cflag&~(tcflag_t)CSIZE | CS7;
	}
}

static void
oddp(int not)
{
	if (not) {
		ts.c_cflag &= ~(tcflag_t)(PARENB|PARODD);
		ts.c_cflag = ts.c_cflag&~(tcflag_t)CSIZE | CS8;
	} else {
		ts.c_cflag |= PARENB|PARODD;
		ts.c_cflag = ts.c_cflag&~(tcflag_t)CSIZE | CS7;
	}
}

static void
spacep(int not)
{
	evenp(not);
}

static void
markp(int not)
{
	oddp(not);
}

static void
raw(int not)
{
	if (not) {
		ts.c_cc[VEOF] = ts.c_cc[VMIN] = '\4';
		ts.c_cc[VEOL] = ts.c_cc[VTIME] = vdis;
		ts.c_oflag |= OPOST;
		ts.c_lflag |= ISIG|ICANON;
	} else {
		ts.c_cc[VEOF] = ts.c_cc[VMIN] = 1;
		ts.c_cc[VEOL] = ts.c_cc[VTIME] = 1;
		ts.c_oflag &= ~(tcflag_t)OPOST;
		ts.c_lflag &= ~(tcflag_t)(ISIG|ICANON);
	}
}

static void
cooked(int not)
{
	if (not)
		inval();
	raw(1);
}

static void
nl(int not)
{
	if (not) {
		ts.c_iflag |= ICRNL;
		ts.c_oflag |= ONLCR;
		ts.c_iflag &= ~(tcflag_t)(INLCR|IGNCR);
		ts.c_oflag &= ~(tcflag_t)(OCRNL|ONLRET);
	} else {
		ts.c_iflag &= ~(tcflag_t)ICRNL;
		ts.c_oflag &= ~(tcflag_t)ONLCR;
	}
}

static void
sane(int not)
{
	speed_t	ispeed, ospeed;

	if (not)
		inval();
	ispeed = cfgetispeed(&ts);
	ospeed = cfgetospeed(&ts);
	ts.c_cc[VINTR] = '\3';
	ts.c_cc[VQUIT] = '\34';
	ts.c_cc[VERASE] = '\10';
	ts.c_cc[VKILL] = '\25';
	ts.c_cc[VEOF] = '\4';
	ts.c_cc[VEOL] = vdis;
	ts.c_cc[VEOL2] = vdis;
#ifdef	VSWTCH
	ts.c_cc[VSWTCH] = vdis;
#endif
	ts.c_cc[VSTART] = '\21';
	ts.c_cc[VSTOP] = '\23';
	ts.c_cc[VSUSP] = '\32';
#ifdef	VREPRINT
	ts.c_cc[VREPRINT] = '\22';
#endif
#ifdef	VDISCARD
	ts.c_cc[VDISCARD] = '\17';
#endif
#ifdef	VWERASE
	ts.c_cc[VWERASE] = '\27';
#endif
	ts.c_cc[VLNEXT] = '\26';
	ts.c_cflag = CS8|CREAD;
	ts.c_lflag = ISIG|ICANON|ECHO|ECHOE|ECHOK|ECHOKE|IEXTEN;
	ts.c_iflag = BRKINT|IGNPAR|ICRNL|IXON|IMAXBEL;
#ifdef	IUTF8
	if (MB_CUR_MAX > 1) {
		wchar_t	wc;
		if (mbtowc(&wc, "\303\266", 2) == 2 && wc == 0xF6 &&
				mbtowc(&wc, "\342\202\254", 3) == 3 &&
				wc == 0x20AC)
			ts.c_iflag |= IUTF8;
	}
#endif	/* IUTF8 */
	ts.c_oflag = OPOST|ONLCR;
	cfsetispeed(&ts, ispeed);
	cfsetospeed(&ts, ospeed);
}

#ifdef	OFDEL
static void
fill(int not)
{
	if (not)
		ts.c_oflag &= ~(tcflag_t)(OFILL|OFDEL);
	else {
		ts.c_oflag |= OFILL;
		ts.c_oflag &= ~(tcflag_t)OFDEL;
	}
}
#endif

#ifdef	XCASE
static void
lcase(int not)
{
	if (not) {
		ts.c_lflag &= ~(tcflag_t)XCASE;
		ts.c_iflag &= ~(tcflag_t)IUCLC;
		ts.c_oflag &= ~(tcflag_t)OLCUC;
	} else {
		ts.c_lflag |= XCASE;
		ts.c_iflag |= IUCLC;
		ts.c_oflag |= OLCUC;
	}
}
#endif

static void
ek(int not)
{
	if (not)
		inval();
	ts.c_cc[VERASE] = '\10';
	ts.c_cc[VKILL] = '\25';
}

#ifdef	TABDLY
static void
tty33(int not)
{
	if (not)
		inval();
	ts.c_oflag &= ~(tcflag_t)(NLDLY|CRDLY|TABDLY|BSDLY|FFDLY|VTDLY);
	ts.c_oflag |= CR1;
}

static void
tty37(int not)
{
	if (not)
		inval();
	ts.c_oflag &= ~(tcflag_t)(NLDLY|CRDLY|TABDLY|BSDLY|FFDLY|VTDLY);
	ts.c_oflag |= NL1|CR2|TAB1|FF1|VT1;
}

static void
vt05(int not)
{
	if (not)
		inval();
	ts.c_oflag &= ~(tcflag_t)(NLDLY|CRDLY|TABDLY|BSDLY|FFDLY|VTDLY);
	ts.c_oflag |= NL1;
}

static void
tn300(int not)
{
	if (not)
		inval();
	ts.c_oflag &= ~(tcflag_t)(NLDLY|CRDLY|TABDLY|BSDLY|FFDLY|VTDLY);
	ts.c_oflag |= CR1;
}

static void
ti700(int not)
{
	if (not)
		inval();
	ts.c_oflag &= ~(tcflag_t)(NLDLY|CRDLY|TABDLY|BSDLY|FFDLY|VTDLY);
	ts.c_oflag |= CR2;
}

static void
tek(int not)
{
	if (not)
		inval();
	ts.c_oflag &= ~(tcflag_t)(NLDLY|CRDLY|TABDLY|BSDLY|FFDLY|VTDLY);
	ts.c_oflag |= FF1;
}
#endif

static void
rows(int not)
{
	if (not)
		inval();
	if (args[1] == NULL)
		return;
	wschange = 1;
	ws.ws_row = atoi(*++args);
}

static void
columns(int not)
{
	if (not)
		inval();
	if (args[1] == NULL)
		return;
	wschange = 1;
	ws.ws_col = atoi(*++args);
}

static void
ypixels(int not)
{
	if (not)
		inval();
	if (args[1] == NULL)
		return;
	wschange = 1;
	ws.ws_ypixel = atoi(*++args);
}

static void
xpixels(int not)
{
	if (not)
		inval();
	if (args[1] == NULL)
		return;
	wschange = 1;
	ws.ws_xpixel = atoi(*++args);
}

static void
vmin(int not)
{
	if (not)
		inval();
	if (args[1] == NULL)
		return;
	ts.c_cc[VMIN] = atoi(*++args);
}

static void
vtime(int not)
{
	if (not)
		inval();
	if (args[1] == NULL)
		return;
	ts.c_cc[VTIME] = atoi(*++args);
}


static void
line(int not)
{
	if (not)
		inval();
	if (args[1] == NULL)
		return;
#ifdef	__linux__
	ts.c_line = atoi(*++args);
#endif
}

static const char gfmt[] ="%lx:%lx:%lx:%lx:"
		"%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x";

static void
glist(void)
{
	printf(gfmt,	(long)ts.c_iflag,
			(long)ts.c_oflag,
			(long)ts.c_cflag,
			(long)ts.c_lflag,
			(int)ts.c_cc[VINTR],
			(int)ts.c_cc[VQUIT],
			(int)ts.c_cc[VERASE],
			(int)ts.c_cc[VKILL],
			(int)ts.c_cc[VEOF],
			(int)ts.c_cc[VEOL],
			(int)ts.c_cc[VEOL2],
#ifdef	VSWTCH
			(int)ts.c_cc[VSWTCH],
#else
			(int)vdis,
#endif
			(int)ts.c_cc[VSTART],
			(int)ts.c_cc[VSTOP],
			(int)ts.c_cc[VSUSP],
#ifdef	VDSUSP
			(int)ts.c_cc[VDSUSP],
#else
			(int)vdis,
#endif
#ifdef	VREPRINT
			(int)ts.c_cc[VREPRINT],
#else
			(int)vdis,
#endif
#ifdef	VDISCARD
			(int)ts.c_cc[VDISCARD],
#else
			(int)vdis,
#endif
#ifdef	VWERASE
			(int)ts.c_cc[VWERASE],
#else
			(int)vdis,
#endif
			(int)ts.c_cc[VLNEXT],
			(int)ts.c_cc[VMIN],
			(int)ts.c_cc[VTIME]);
	putchar('\n');
}

static void
gset(void)
{
	long	iflag, oflag, cflag, lflag;
	int	vintr, vquit, verase, vkill,
		veof, veol, veol2, vswtch,
		vstart, vstop, vsusp, vdsusp,
		vrprnt, vflush, vwerase, vlnext,
		vmin, vtime;

	if (sscanf(*args, gfmt,
			&iflag, &oflag, &cflag, &lflag,
			&vintr, &vquit, &verase, &vkill,
			&veof, &veol, &veol2, &vswtch,
			&vstart, &vstop, &vsusp, &vdsusp,
			&vrprnt, &vflush, &vwerase, &vlnext,
			&vmin, &vtime) != 22)
		inval();
	ts.c_iflag = iflag;
	ts.c_oflag = oflag;
	ts.c_cflag = cflag;
	ts.c_lflag = lflag;
	ts.c_cc[VINTR] = vintr;
	ts.c_cc[VQUIT] = vquit;
	ts.c_cc[VKILL] = vkill;
	ts.c_cc[VEOF] = veof;
	ts.c_cc[VEOL] = veol;
	ts.c_cc[VEOL2] = veol2;
#ifdef	VSWTCH
	ts.c_cc[VSWTCH] = vswtch;
#endif
	ts.c_cc[VSTART] = vstart;
	ts.c_cc[VSTOP] = vstop;
	ts.c_cc[VSUSP] = vsusp;
#ifdef	VDSUSP
	ts.c_cc[VDSUSP] = vdsusp;
#endif
#ifdef	VREPRINT
	ts.c_cc[VREPRINT] = vrprnt;
#endif
#ifdef	VDISCARD
	ts.c_cc[VDISCARD] = vflush;
#endif
#ifdef	VWERASE
	ts.c_cc[VWERASE] = vwerase;
#endif
	ts.c_cc[VLNEXT] = vlnext;
	ts.c_cc[VMIN] = vmin;
	ts.c_cc[VTIME] = vtime;
}

#ifdef	UCB
static void
hchar(int c, int c2, int spc)
{
	int	n = 0;

chr:	if (c != vdis) {
		if (c & 0200) {
			c &= 0177;
			n += printf("M-");
		}
		if ((c&037) == c)
			n += printf("^%c", c | 0100);
		else if (c == '\177')
			n += printf("^?");
		else {
			putchar(c);
			n++;
		}
	}
	if (c2 != EOF) {
		putchar('/');
		n++;
		c = c2;
		c2 = EOF;
		goto chr;
	}
	if (spc)
		while (n++ < 7)
			putchar(' ');
}

static void
hlist(int aflag)
{
	list(aflag, 1);
	printf("erase  kill   werase rprnt  flush  lnext  "
			"susp   intr   quit   stop   eof\n");
	hchar(ts.c_cc[VERASE]&0377, EOF, 1);
	hchar(ts.c_cc[VKILL]&0377, EOF, 1);
#ifdef	VWERASE
	hchar(ts.c_cc[VWERASE]&0377, EOF, 1);
#else
	hchar(vdis, EOF, 1);
#endif
#ifdef	VREPRINT
	hchar(ts.c_cc[VREPRINT]&0377, EOF, 1);
#else
	hchar(vdis, EOF, 1);
#endif
#ifdef	VDISCARD
	hchar(ts.c_cc[VDISCARD]&0377, EOF, 1);
#else
	hchar(vdis, EOF, 1);
#endif
	hchar(ts.c_cc[VLNEXT]&0377, EOF, 1);
	hchar(ts.c_cc[VSUSP]&0377, EOF, 1);
	hchar(ts.c_cc[VINTR]&0377, EOF, 1);
	hchar(ts.c_cc[VQUIT]&0377, EOF, 1);
	hchar(ts.c_cc[VSTOP]&0377, ts.c_cc[VSTART]&0377, 1);
	hchar(ts.c_cc[VEOF]&0377, EOF, 1);
	putchar('\n');
}

static void
speed(void)
{
	printf("%s\n", baudrate(cfgetospeed(&ts)));
}

static void
size(void)
{
	printf("%d %d\n", (int)ws.ws_row, (int)ws.ws_col);
}

static void
litout(int not)
{
	if (not) {
		ts.c_cflag |= PARENB;
		ts.c_iflag |= ISTRIP;
		ts.c_oflag |= OPOST;
		ts.c_cflag = ts.c_cflag&~(tcflag_t)CSIZE | CS7;
	} else {
		ts.c_cflag &= ~(tcflag_t)PARENB;
		ts.c_iflag &= ~(tcflag_t)ISTRIP;
		ts.c_oflag &= ~(tcflag_t)OPOST;
		ts.c_cflag = ts.c_cflag&~(tcflag_t)CSIZE | CS8;
	}
}

static void
pass8(int not)
{
	if (not) {
		ts.c_cflag |= PARENB;
		ts.c_iflag |= ISTRIP;
		ts.c_cflag = ts.c_cflag&~(tcflag_t)CSIZE | CS7;
	} else {
		ts.c_cflag &= ~(tcflag_t)PARENB;
		ts.c_iflag &= ~(tcflag_t)ISTRIP;
		ts.c_cflag = ts.c_cflag&~(tcflag_t)CSIZE | CS8;
	}
}

static void
crt(int not)
{
	if (not)
		inval();
	ts.c_lflag |= ECHOE|ECHOCTL;
	if (cfgetospeed(&ts) >= B1200)
		ts.c_lflag |= ECHOKE;
}

static void
dec(int not)
{
	if (not)
		inval();
	ts.c_cc[VERASE] = '\177';
	ts.c_cc[VKILL] = '\25';
	ts.c_cc[VINTR] = '\3';
	ts.c_iflag &= ~(tcflag_t)IXANY;
	crt(not);
}
#endif	/* UCB */
