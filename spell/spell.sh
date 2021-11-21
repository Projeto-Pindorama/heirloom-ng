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


#	Copyright (c) 1998, 2000 by Sun Microsystems, Inc.
#	All rights reserved.

#	Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany

# from OpenSolaris "spell.sh	1.21	05/06/08 SMI"	/* SVr4.0 1.11	*/
#	spell program
# Sccsid @(#)spell.sh	2.1 (gritter) 6/21/05
# B_SPELL flags, D_SPELL dictionary, F_SPELL input files, H_SPELL history,
# S_SPELL stop, V_SPELL data for -v
# L_SPELL sed script, I_SPELL -i option to deroff
PATH=@DEFLIB@/spell:@SV3BIN@:@DEFBIN@:$PATH export PATH

H_SPELL=${H_SPELL:-@SPELLHIST@}
V_SPELL=/dev/null
F_SPELL=
FT_SPELL=
B_SPELL=
L_SPELL="sed -e \"/^[.'].*[.'][ 	]*nx[ 	]*\/usr\/lib/d\" -e \"/^[.'].*[.'][ 	]*so[ 	]*\/usr\/lib/d\" -e \"/^[.'][ 	]*so[ 	]*\/usr\/lib/d\" -e \"/^[.'][ 	]*nx[ 	]*\/usr\/lib/d\" "

LOCAL=

# mktmpdir - Create a private (mode 0700) temporary directory inside of /tmp
# for this process's temporary files.  We set up a trap to remove the
# directory on exit (trap 0), and also on SIGHUP, SIGINT, SIGQUIT, and
# SIGTERM.
#
mktmpdir() {
	tmpdir=/tmp/spell.$$
	trap "rm -rf $tmpdir; exit" 0 1 2 13 15
	mkdir -m 700 $tmpdir || exit 1
}

mktmpdir

DEROFF="deroff \$I_SPELL"

while :
do
	while getopts abvxli A
	do
		case $A in
		v)	B_SPELL="$B_SPELL -v"
			V_SPELL=$tmpdir/spell.$$
			;;
		b)	D_SPELL=${LB_SPELL:-@DEFLIB@/spell/hlistb}
			B_SPELL="$B_SPELL -b" ;;
		a)	;;
		x)	B_SPELL="$B_SPELL -x" ;;
		l)	L_SPELL="cat" ;;
		i)	I_SPELL="-i" ;;
		*)	echo "usage: `basename $0` [-v] [-b] [-x] [-l] [-i] [+local_file] [ files ]" >&2
			exit 2 ;;
		esac
	done
	test $OPTIND -gt 1 && shift `expr $OPTIND - 1`
	OPTIND=1
	case $1 in
	+*)
		LOCAL=`expr x"$1" : 'x+\(.*\)'`
		if test -z "$LOCAL"
		then
			echo "`basename $0` cannot identify local spell file" >&2
			exit 1
		elif test ! -r "$LOCAL"
		then
			echo "`basename $0` cannot read $LOCAL" >&2
			exit 1
		fi
		shift
		;;
	*)
		break
	esac
done

(cat ${@+"$@"}; echo) | eval $L_SPELL |\
 deroff $I_SPELL |\
 LC_ALL=C tr -cs "[A-Z][a-z][0-9]\'\&\.\,\;\?\:" "[\012*]" |\
 sed '1,$s/^[^A-Za-z0-9]*//' | sed '1,$s/[^A-Za-z0-9]*$//' |\
 sed -n "/[A-Za-z]/p" | sort -u +0 |\
 spellprog ${S_SPELL:-@DEFLIB@/spell/hstop} 1 |\
 spellprog $B_SPELL ${D_SPELL:-@DEFLIB@/spell/hlista} $V_SPELL |\
 comm -23 - ${LOCAL:-/dev/null} |\
 tee -a $H_SPELL
who am i >>$H_SPELL 2>/dev/null
case $V_SPELL in
/dev/null)
	exit
esac
sed '/^\./d' $V_SPELL | sort -u +1f +0
