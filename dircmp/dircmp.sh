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
#	Copyright (c) 1996-2000 by Sun Microsystems, Inc.
#	All rights reserved.
# from OpenSolaris "dircmp.sh	1.21	05/06/08 SMI"
#     Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
#     Sccsid @(#)dircmp.sh	1.6 (gritter) 7/1/05
tmpdir=/var/tmp
PATH=@SV3BIN@:@DEFBIN@:$PATH export PATH
progname=`basename $0`
USAGE="$progname: usage: $progname -s -d -wn directory directory"
trap "rm -f $tmpdir/dc$$*;exit" 1 2 3 15
exitstat=0
sizediff=
cmpdiff=
Sflag=0
Dflag=0
fsize1=
fsize2=
LFBOUND=2147483648
width=72

#
# function to generate consistent "diff" output whether or not files are intact
#
dodiffs() {
	type=`LC_ALL=C file "$D1/$a"`
	case "$type" in
		*text)  ;;
		*script) ;;
		*empty*) echo $D1/`basename "$a"` is an empty file |
			  pr -h "diff of $a in $D1 and $D2" >> $tmpdir/dc$$g
			continue
        	;;
        	*cannot*) echo $D1/`basename "$a"` does not exist |
			  pr -h "diff of $a in $D1 and $D2" >> $tmpdir/dc$$g
			continue
        	;;
        	*)	echo $D1/`basename "$a"` is an object file |
		   	  pr -h "diff of $a in $D1 and $D2" >> $tmpdir/dc$$g
			continue
        	;;
	esac
	type=`LC_ALL=C file "$D2/$a"`
	case "$type" in
        	*text)  ;;
        	*script) ;;
        	*empty*) echo $D2/`basename "$a"` is an empty file |
			  pr -h "diff of $a in $D1 and $D2" >> $tmpdir/dc$$g
			continue
        	;;
        	*cannot*) echo $D2/`basename "$a"` does not exist |
			  pr -h "diff of $a in $D1 and $D2" >> $tmpdir/dc$$g
			continue
        	;;
        	*)	echo $D2/`basename "$a"` is an object file |
			  pr -h "diff of $a in $D1 and $D2" >> $tmpdir/dc$$g
			continue
        	;;
	esac
	#   
	# If either is a "large file" use bdiff (LF aware),
	# else use diff.
	#
	if test $fsize1 -lt $LFBOUND -a $fsize2 -lt $LFBOUND
	then cmd="diff"
	else cmd="bdiff"
	fi
	($cmd "$D1/$a" "$D2/$a"; echo $? > $tmpdir/dc$$status) | \
	    pr -h "diff of $a in $D1 and $D2" >> $tmpdir/dc$$g
	if test `cat $tmpdir/dc$$status` -ne 0
	then exitstat=$diffstat
	fi
}
#
# dircmp entry point
#
while getopts dsw: i
do
	case $i in
	d)	Dflag=1;; 
	s)	Sflag=1;; 
	w)	width=`expr $OPTARG + 0 2>/dev/null`
		if [ $? = 2 ]
		then echo "$progname: numeric argument required"
			exit 2
		fi
		;;
	*)	echo $USAGE
		exit 2;;
	esac
done
shift `expr $OPTIND - 1`
#
D0=`pwd`
D1=$1
D2=$2
if [ $# -lt 2 ]
then echo $USAGE
     exit 1
elif [ ! -d "$D1" ]
then echo $D1 not a directory !
     exit 2
elif [ ! -d "$D2" ]
then echo $D2 not a directory !
     exit 2
fi
#
# find all dirs/files in both directory hierarchies. Use "comm" to identify
# which are common to both hierarchies as well as unique to each.
# At this point, print those that are unique.
#
cd "$D1"
find . -name '*[
\\]*' -prune -o ! -name '*[
\\]*' -print | sort > $tmpdir/dc$$a
cd "$D0"
cd "$D2"
find . -name '*[
\\]*' -prune -o ! -name '*[
\\]*' -print | sort > $tmpdir/dc$$b
comm $tmpdir/dc$$a $tmpdir/dc$$b | sed -n \
	-e "/^		/w $tmpdir/dc$$c" \
	-e "/^	[^	]/w $tmpdir/dc$$d" \
	-e "/^[^	]/w $tmpdir/dc$$e"
rm -f $tmpdir/dc$$a $tmpdir/dc$$b
pr -w${width} -h "$D1 only and $D2 only" -m $tmpdir/dc$$e $tmpdir/dc$$d
rm -f $tmpdir/dc$$e $tmpdir/dc$$d
#
# Generate long ls listings for those dirs/files common to both hierarchies.
# Use -lgn to avoid problem when user or group names are too long, causing
# expected field separator to be missing
# Avoid other potential problems by piping through sed:
#  - Remove: Spaces in size field for block & character special files
#      '71, 0' becomes '710'
#  - For file name, do not print '-> some_file'
#      '/tmp/foo -> FOO' becomes '/tmp/foo'

# The following sed is to read filenames with special characters
sed -e 's/..//'  -e  's/\([^-a-zA-Z0-9/_.]\)/\\\1/g' < $tmpdir/dc$$c > $tmpdir/dc$$f


cat $tmpdir/dc$$f | xargs ls -lLgnd | \
  sed -e '/^[bc]/ s/, *//' -e '/^l/ s/ -> .*//' > $tmpdir/dc$$i 2>/dev/null
cd "$D0"
cd "$D1"


cat $tmpdir/dc$$f | xargs ls -lLgnd | \
sed -e '/^[bc]/ s/, *//' -e '/^l/ s/ -> .*//' > $tmpdir/dc$$h 2>/dev/null
cd "$D0"
> $tmpdir/dc$$g
#
# Process the results of the 'ls -lLgnd' to obtain file size info
# and identify a large file's existence.
#
while read <&3 tmp tmp tmp fsize1 tmp tmp tmp a &&
      read <&4 tmp tmp tmp fsize2 tmp tmp tmp b; do
	#
	# A window of opportunity exists where the ls -lLgnd above
	# could produce different
	# results if any of the files were removed since the find command.
	# If the pair of reads above results in different values (file names) for 'a'
	# and 'b', then get the file pointers in sync before continuing, and display
	# "different" message as customary.
	#
	if test "x$a" != "x$b"; then
	while test "$a" -lt "$b"; do
		if test $Sflag -ne 1
		then echo "different	$a"
		dodiffs
		fi
		read <&3 tmp tmp tmp fsize1 tmp tmp tmp a
	done
	while test "$a" -gt "$b"; do
		if test $Sflag -ne 1
		then echo "different	$b"
		dodiffs
		fi
		read <&4 tmp tmp tmp fsize2 tmp tmp tmp b
	done
	fi
	cmpdiff=0
	sizediff=0
	if [ -d "$D1/$a" ]
	then if test $Sflag -ne 1
	     then echo "directory	$a"
	     fi
	elif [ -f "$D1/$a" ]
	then 
	     #
	     # If the file sizes are different, then we can skip the run
	     # of "cmp" which is only used to determine 'same' or 'different'.
	     # If the file sizes are the same, we still need to run "cmp"
	     #
	     if test $fsize1 -ne $fsize2
	     then
		sizediff=1
	     else
		cmp -s -- "$D1/$a" "$D2/$a"
		cmpdiff=$?
	     fi
	     if test $sizediff -eq 0 -a $cmpdiff -eq 0
	     then if test $Sflag -ne 1
		  then echo "same     	$a"
		  fi
	     else echo "different	$a"
		  if test $Dflag -eq 1
		  then
			dodiffs
		  fi
	     fi
	elif test $Sflag -ne 1
	then echo "special  	$a"
	fi
done 3<$tmpdir/dc$$h 4<$tmpdir/dc$$i | pr -r -h "Comparison of $D1 $D2"
if test $Dflag -eq 1
then cat $tmpdir/dc$$g
fi
rm -f $tmpdir/dc$$*
exit $exitstat
