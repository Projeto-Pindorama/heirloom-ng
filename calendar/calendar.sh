#
#	from Unix 32V /usr/src/cmd/calendar/calendar.sh
#
# Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
#
# SPDX-Licence-Identifier: Caldera
#
# Sccsid @(#)calendar.sh	1.8 (gritter) 5/29/03

PATH=@DEFBIN@:@SV3BIN@:$PATH export PATH
calprog=@DEFLIB@/calprog
case $# in
0)
	if test -r calendar
	then
		egrep -e "`$calprog`" calendar
	else
		echo "`basename $0`: `pwd`/calendar not found" >&2
		exit 1
	fi
	;;
*)
	tmp=/tmp/cal$$
	trap "rm -f $tmp; trap '' 0; exit" 0 1 2 3 13 15
	logins -uxo | while IFS=: read user uid group gid gecos dir shell rest
	do
		if test -f $dir/calendar -a -r $dir/calendar
		then
			egrep -e "`$calprog`" $dir/calendar >$tmp 2>/dev/null
			test -s $tmp && mail $user <$tmp
		fi
	done
esac
