#!/sbin/sh

# Sccsid @(#)maninst.sh	1.8 (gritter) 01/27/07

usage() {
	echo "usage: $1 [-c] [-g group] [-m mode] [-o owner] file destination" >&2
	exit 2
}

owner= mode= group=

while getopts cg:m:o: arg
do
	case $arg in
	c)	;;
	g)	group=$OPTARG ;;
	o)	owner=$OPTARG ;;
	m)	mode=$OPTARG ;;
	*)	usage $0 ;;
	esac
done

test $OPTIND -gt 1 && shift `expr $OPTIND - 1`
test $# != 3 || usage $0

<"$1" >"$2" sed '
	s,/usr/5lib,@DEFLIB@,g
	t
	s,@.DEFBIN.@,@DEFBIN@,g
	t
	s,/usr/5bin/s42,@S42BIN@,g
	t
	s,/usr/5bin/posix2001,@SU3BIN@,g
	t
	s,/usr/5bin/posix,@SUSBIN@,g
	t
	s,/usr/5bin,@SV3BIN@,g
	t
	s,/usr/ucb,@UCBBIN@,g
	t
	s,/usr/ccs/bin,@CCSBIN@,g
	t
	s,/var/adm/spellhist,@SPELLHIST@,g
	t
	s,/etc/default,@DFLDIR@,g
	t
	s,/usr/5lib/magic,@MAGIC@,g
	t
'

test "x$owner" != x && chown "$owner" "$2"
test "x$group" != x && chgrp "$group" "$2"
test "x$mode" != x && chmod "$mode" "$2"
