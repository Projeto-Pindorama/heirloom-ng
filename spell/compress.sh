#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License, Version 1.0 only
# (the "License").  You may not use this file except in compliance
# with the License.
#
# You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
# or http://www.opensolaris.org/os/licensing.
# See the License for the specific language governing permissions
# and limitations under the License.
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file at usr/src/OPENSOLARIS.LICENSE.
# If applicable, add the following below this CDDL HEADER, with the
# fields enclosed by brackets "[]" replaced with your own identifying
# information: Portions Copyright [yyyy] [name of copyright owner]
#
# CDDL HEADER END
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
