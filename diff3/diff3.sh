#
# This code contains changes by
# Gunnar Ritter, Freiburg i. Br., Germany, March 2003. All rights reserved.
#
# Conditions 1, 2, and 4 and the no-warranty notice below apply
# to these changes.
#
#
# Copyright (c) 1991
#      The Regents of the University of California.  All rights reserved.
#
# SPDX-Licence-Identifier: BSD-4-Clause-UC 
#
#
# Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
# 
# SPDX-Licence-Identifier: Caldera
# 
#
#	from 4.3BSD diff3.sh	4.1	83/02/10
#
#	Sccsid @(#)diff3.sh	1.7 (gritter) 5/29/03
#
PATH=@DEFBIN@:@SV3BIN@:$PATH export PATH
unset e
case $1 in
-*)
	e=$1
	shift;;
esac
if test $# = 3 && test -r "$1" && test -r "$2" && test -r "$3"
then
	:
else
	echo "usage: `basename $0` file1 file2 file3" 1>&2
	exit 2
fi
trap "rm -f /tmp/d3[abcdef]$$" 0 1 2 13 15
if test -f "$1" || test -b "$1"
then
	file1=$1
else
	cat -- "$1" >/tmp/d3c$$ || exit
	file1=/tmp/d3c$$
fi
if test -f "$2" || test -b "$2"
then
	file2=$2
else
	cat -- "$2" >/tmp/d3d$$ || exit
	file2=/tmp/d3d$$
fi
if test -f "$3" || test -b "$3"
then
	file3=$3
else
	cat -- "$3" >/tmp/d3e$$ || exit
	file3=/tmp/d3e$$
fi
diff -- "$file1" "$file3" >/tmp/d3a$$
diff -- "$file2" "$file3" >/tmp/d3b$$
@DEFLIB@/diff3prog ${e+"$e"} /tmp/d3[ab]$$ "$file1" "$file2" "$file3"
