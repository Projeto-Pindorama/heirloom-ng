#
#	from Unix 32V /usr/src/cmd/calendar/calendar.sh
#
# Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#   Redistributions of source code and documentation must retain the
#    above copyright notice, this list of conditions and the following
#    disclaimer.
#   Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#   All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#      This product includes software developed or owned by Caldera
#      International, Inc.
#   Neither the name of Caldera International, Inc. nor the names of
#    other contributors may be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# USE OF THE SOFTWARE PROVIDED FOR UNDER THIS LICENSE BY CALDERA
# INTERNATIONAL, INC. AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL CALDERA INTERNATIONAL, INC. BE
# LIABLE FOR ANY DIRECT, INDIRECT INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
# BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
# EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
