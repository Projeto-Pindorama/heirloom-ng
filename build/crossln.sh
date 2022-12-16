#!/sbin/sh

# Sccsid @(#)crossln.sh	1.4 (gritter) 3/10/05

# Create a relative symbolic link.

usage() {
	echo "usage: $1 target linkname [root]" >&2
	exit 2
}

doit() {
	rm -f -- "$2" || exit
	exec @LNS@ -- "$1" "$2" || exit
}

test $# != 2 -a $# != 3 && usage $0

case $3 in
'')	;;
*/)	;;
*)	set -- "$1" "$2" "$3/"
	;;
esac

case $2 in
/*)	;;
..|../*)	echo "cannot resolve ../*" >&2; exit 1 ;;
.)	set -- "$1" "`pwd`" ${3+"$3"};;
*)	set -- "$1" "`pwd`/$2" ${3+"$3"};;
esac

case $1 in
/*)	;;
*)	doit "$1" "$2"
	;;
esac

test -d "$2" && test ! -d "$1" && set -- "$1" "$2/`basename $1`" ${3+"$3"}

tgt=`awk </dev/null 'BEGIN {
	s1="'"$1"'"
	s2="'"$2"'"
	for (;;) {
		i1 = index(s1, "/")
		i2 = index(s2, "/")
		if (i1 == 0 || i1 != i2 || \
				substr(s1, 1, i1) != substr(s2, 1, i2))
			break
		s1 = substr(s1, i1 + 1)
		s2 = substr(s2, i2 + 1)
	}
	n = 0
	for (i = 1; i <= length(s2); i++) {
		if (substr(s2, i, 1) == "/") {
			n++
			while (substr(s2, i+1, 1) == "/")
				i++
		}
	}
	while (n-- > 0)
		s1 = ("../" s1)
	print s1
}'`

doit "$tgt" "$2"
