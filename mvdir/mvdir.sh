#
# SPDX-Licence-Identifier: CDDL-1.0
#

#
# Copyright 1997 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#

#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved


# from OpenSolaris "mvdir.sh	1.11	05/06/08 SMI"

#     Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
#     Sccsid @(#)mvdir.sh	1.8 (gritter) 7/1/05

PATH=@SV3BIN@:@DEFBIN@:$PATH export PATH
if [ $# != 2 ]
then
  echo "Usage: `basename $0` fromdir newname" >&2
  exit 2
fi
if [ "$1" = . ]
then
	echo "`basename $0`: cannot move '.'" >&2
	exit 2
fi
f=`basename "$1"`
t="$2"
if [ -d "$t" ]
then
	t="$t"/"$f"
fi
if [ -f "$t"  -o -d "$t" ]
then
  echo "$t" exists >&2
  exit 1
fi
if [  ! -d "$1" ]
then
  echo "$1" must be a directory >&2
  exit 1
fi

# *** common path tests: The full path name for $1 must not
# *** 			 be an anchored substring of the full
# ***			 path name for $2
(
	(cd "$1"; pwd)
	mkdir -- "$t" || exit	# see if we can create the directory
	(cd "$t"; pwd)
	rmdir -- "$t"
	echo		# for synchronization
	echo		# signal error to awk
	echo		# in case of two errors
) | LC_ALL=C awk '{
	from = $0
	getline
	to = $0
	getline
	if (to == "" || from == "")
		exit 2
	if (index(to, from) == 1 || index(from, to) == 1)
		exit 1
	exit 0
}'

case $? in
0)	;;
1)	echo arguments have common path >&2
	exit 1
	;;
*)	exit $?
esac
# ***

mv -- "$1" "$t"
