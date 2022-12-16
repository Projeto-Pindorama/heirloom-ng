#
# SPDX-Licence-Identifier: CDDL-1.0
#
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved
#
#	Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany


# from OpenSolaris "compress.sh	1.7	05/06/08 SMI"	/* SVr4.0 1.3	*/
#	Sccsid @(#)compress.sh	2.1 (gritter) 6/21/05
#	compress - compress the spell program log

PATH=@SV3BIN@:@DEFBIN@:$PATH export PATH

tmp=/var/tmp/spellhist

trap "rm -f $tmp;exit" 0 1 2 3 15
echo "COMPRESSED `date`" > $tmp
grep -v ' ' @SPELLHIST@ | sort -fud >> $tmp
cp -f $tmp @SPELLHIST@
