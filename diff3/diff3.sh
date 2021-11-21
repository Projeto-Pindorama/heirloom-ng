#
# This code contains changes by
# Gunnar Ritter, Freiburg i. Br., Germany, March 2003. All rights reserved.
#
# Conditions 1, 2, and 4 and the no-warranty notice below apply
# to these changes.
#
#
# Copyright (c) 1991
# 	The Regents of the University of California.  All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
# 	This product includes software developed by the University of
# 	California, Berkeley and its contributors.
# 4. Neither the name of the University nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
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
