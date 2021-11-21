
#
# Routines common to all man sub-commands.
#
#	Sccsid common.ksh	1.37 (gritter) 8/12/05
#

#
# whatis/apropos sub-commands. $1 is command type, all following are
# names to search for.
#
whatpropos() {
	status=1
	case "$1" in
	apropos)
		cmd=apropos
		;;
	whatis)
		cmd=whatis
		;;
	esac
	shift
	for name
	do
		visited=:
		test "$cmd" = whatis && name=${name##*/}
		manpath=":$MANPATH:"
		while dir="${manpath%%:*}"; manpath="${manpath#*:}"
			test -n "$dir" || test -n "$manpath"
		do
			test -z "$dir" && continue
			case "$visited" in
			*:"$dir"*(/):*) continue
			esac
			visited="$visited$dir:"
			test -d "$dir" && cd "$dir" 2>/dev/null || {
				test -n "$specpath" &&
				echo "$dir is not accessible." >&2; continue;
			}
			test -n "$debug" && echo "mandir path = $dir"
			if test -n "$system"
			then
				test -d "$system" && cd "$system" 2>/dev/null ||
					continue
				dir="$dir/$system"
			fi
			if test -r windex
			then
				case "$cmd" in
				apropos)
					egrep -i "$name" windex && status=0
					;;
				whatis)
					egrep "^$name[ 	]+" windex && status=0
					;;
				esac
			elif test -r whatis
			then
				case "$cmd" in
				apropos)
					egrep -i "$name" whatis && status=0
					;;
				whatis)
					egrep "^$name[ 	]+\(" whatis && status=0
					;;
				esac
			fi
		done
	done
	exit $status
}

#
# Format the page given in $1. $2 gives the format (man, cat), $3 gives the
# section number, and $4 gives an original name possibly different from the
# actual page name. The result is stored in a temporary file.
#
format() {
	test -r "$1" || return
	test "$progname" != catman && test -n "$debug" &&
		echo "    selected = $1"
	if test -n "$lflag"
	then
		echo "$name ($3)	-M $dir"
	elif test "$progname" != catman && test -n "$wflag"
	then
		echo "$dir/$1"
	else
		f_msgdone=
		case "$1" in
		*.gz)
			ZIP='gzip -cd'
			;;
		*.Z)
			ZIP='zcat'
			;;
		*.z)
			ZIP='pcat'
			;;
		*.bz2)
			ZIP='bzcat'
			;;
		*)
			unset ZIP
			;;
		esac
		if test "$2" = man.Z
		then
			zcat < "$1" > /tmp/man$$ || return
			f_file=/tmp/man$$
		elif test -n "$ZIP"
		then
			$ZIP "$1" > /tmp/man$$ || return
			f_file=/tmp/man$$
		else
			f_file="$1"
		fi
		f_cmd=
		case "$2" in
		man|sgml|man.Z)
			test "$progname" != catman &&
				test -z "$tflag" && test -z "$debug" &&
				f_msgdone=' done' &&
				$printn 'Reformatting page.  Wait...' >&2
			if test "$2" = sgml
			then
				$SGML $f_file >/tmp/sman_$$
				test "$f_file" = /tmp/man$$ && rm -f /tmp/man$$
				f_file=/tmp/sman_$$
			fi
			f_cmd="cat $f_file |"
			IFS= read -r f_firstline < "$f_file" || return
			test "$progname" != catman &&
				test -n "$debug" &&
				echo "    firstline = $f_firstline"
			case "$f_firstline" in
			\'\\\"\ +([ertv]))
				while test -n "$f_firstline"
				do
					case "$f_firstline" in
					e*)
						if test -n "$tflag"
						then
							f_cmd="$f_cmd $EQN |"
						else
							f_cmd="$f_cmd $NEQN |"
						fi
						;;
					r*)
						f_cmd="$f_cmd $REFER |"
						;;
					t*)
						f_cmd="$f_cmd $TBL |"
						;;
					v*)
						f_cmd="$f_cmd $VGRIND |"
						;;
					esac
					f_firstline="${f_firstline#?}"
				done
				;;
			.so\ *)
				f_so="${f_firstline#.so+( )}"
				case $f_so in
				*/*)	;;
				*)	f_so=${1%%/*}/$f_so ;;
				esac
				for f_i in "$f_so"*
				do
					format "$f_i" "$2" "$3" "$4"
					break
				done
				return
				;;
			esac
			if test -n "$tflag"
			then
				f_cmd="$f_cmd $TROFF"
			else
				f_cmd="$f_cmd $NROFF"
			fi
			f_cmd="$f_cmd $MACSET"
			if test -z "$tflag" -a -n "$COL"
			then
				f_cmd="$f_cmd | $COL"
			elif test -n "$tflag" -a -n "$TCAT"
			then
				f_cmd="$f_cmd | $TCAT"
			fi
			;;
		html)
			test "$f_file" = "/tmp/man$$" &&
				{	mv /tmp/man$$ /tmp/man$$.html;
					f_file="$f_file.html"; }
			f_cmd="$HTML1 $f_file $HTML2"
			;;
		*)
			f_cmd="cat $f_file"
			;;
		esac
		if test "$progname" != catman
		then
			test -n "$debug" && echo "      executing = $f_cmd"
			texts="$texts /tmp/mp$textno.$$"
			eval "$f_cmd" >/tmp/mp$textno.$$
			textno=$((textno+1))
		else
			f_real="${4%?(.gz|.z|.Z|.bz2)}"
			if test -z "$nflag"
			then
				windexadd "$f_real" "$f_file" "$3"
			fi
			if test -z "$wflag"
			then
				f_cat="${f_real#man}"
				f_cat="cat$f_cat"
				 { test ! -r "cat$3/$f_real" ||
						test "$1" -nt "$f_cat" ; } &&
					if test -z "$debug"
					then
						eval "$f_cmd" > "$f_cat"
					else
						echo "$f_cmd > $f_cat"
					fi
			fi
		fi
		test "$f_file" = /tmp/man$$ && rm -f /tmp/man$$
		test "$f_file" = /tmp/sman_$$ && rm -f /tmp/sman_$$
		test "$f_file" = /tmp/man$$.html && rm -f /tmp/man$$.html
		test -n "$f_msgdone" && echo "$f_msgdone" >&2
	fi
	gotcha="$gotcha$3,"
}

#
# For each argument, try if it is a manual subdirectory and if yes, look
# for matching pages inside.
#
trydir() {
	for t_dir
	do
		test -d "$t_dir" || continue
		test -n "$debug" && echo "  scanning = $t_dir"
		case "$t_dir" in
		cat*)
			test -n "$tflag" && continue
			t_sect="${t_dir#cat}"
			t_type=cat
			;;
		man*.Z)
			t_sect="${t_dir#man}"
			t_sect="${t_sect%.Z}"
			t_type=man.Z
			;;
		man*)
			t_sect="${t_dir#man}"
			t_type=man
			;;
		sman*)
			t_sect="${t_dir#sman}"
			t_type=sgml
			;;
		html.*)
			t_sect="${t_dir#html.}"
			t_type=html
			;;
		*)
			t_sect="$t_dir"
			t_type=unknown
			;;
		esac
		for t_file in "$t_dir"/"$name"."$t_sect"* "$t_dir"/"$name".0
		do
			if test "$t_type" = man && test -z "$tflag"
			then
				t_cat="cat${t_file#man}"
				test -r "$t_cat" &&
					test "$t_cat" -nt "$t_file" && continue
			fi
			format "$t_file" "$t_type" "$t_sect" "$t_file"
			test -z "$aflag" -a "$gotcha" != , && break 2
		done
	done
}

#
# Called when a signal is caught.
#
cleanup() {
	test -n "$texts" && rm -f $texts
	test -f /tmp/man$$ && rm -f /tmp/man$$
	test -f /tmp/man$$.html && rm -f /tmp/man$$.html
	test -f /tmp/mw$$ && rm -f /tmp/mw$$
	test -f /tmp/mi$$ && rm -f /tmp/mi$$
	test -f /tmp/mj$$ && rm -f /tmp/mj$$
	exit
}

usage() {
	case "$progname" in
	whatis|apropos)
		echo "$progname what?" >&2
		;;
	catman)
		echo "usage:	$progname [-p] [-n] [-w] [-M path] [-T man] [sections]" >&2
		;;
	*)
		echo "Usage:	$progname [-] [-t] [-M path] [-T man] [ section ] name ..." >&2
		echo "	$progname -k keyword ..." >&2
		echo "	$progname -f file ..." >&2
		;;
	esac
	exit 2
}

progname=${0##*/} oldpwd="$PWD"
debug= texts= section= system= ontty= specpath=
aflag= dflag= fflag= kflag= lflag= nflag= tflag= wflag=
if test "x$1" = x-
then
	shift
elif test -t 1
then
	ontty=yes
fi
case "$progname" in
whatis|apropos)
	optstring=
	;;
catman)
	optstring=npwM:T:
	LC_ALL=C export LC_ALL
	;;
*)
	optstring=adfklM:m:ps:tT:w
	;;
esac
while getopts ":$optstring" arg
do
	case "$arg" in
	a)
		aflag=1
		;;
	d)
		dflag=1
		;;
	p)
		debug=1
		;;
	f)
		fflag=1
		;;
	k)
		kflag=1
		;;
	l)
		lflag=1 aflag=1
		;;
	M)
		MANPATH="$OPTARG" specpath=1
		;;
	m)
		system="$OPTARG"
		;;
	n)
		nflag=1
		;;
	s)
		lowercase section "$OPTARG"
		;;
	t)
		tflag=1
		;;
	T)
		MACSET="$OPTARG"
		;;
	w)
		wflag=1
		;;
	:)
		echo "$progname: option requires an argument -- $OPTARG" >&2
		usage
		;;
	*)
		echo "$progname: illegal option -- $OPTARG" >&2
		usage
		;;
	esac
done
shift $(($OPTIND - 1))
test "$progname" != catman && test -z "$1" && usage

#
# Read the global configuration file. We cannot simply source it since
# values inside shall be overridden by the inherited environment.
#
if test -r "$DFLDIR/man"
then
	while IFS= read -r line
	do
		case "$line" in
		\#*|'')
			;;
		MANPATH=*)
			test -z "$MANPATH" && MANPATH="${line#*=}" ||
				specpath=1 DEFMANPATH="${line#*=}"
			;;
		TROFF=*)
			test -z "$TROFF" && TROFF="${line#*=}"
			;;
		NROFF=*)
			test -z "$NROFF" && NROFF="${line#*=}"
			;;
		COL=*)
			test -z "$COL" && COL="${line#*=}"
			;;
		TCAT=*)
			test -z "$TCAT" && TCAT="${line#*=}"
			;;
		PAGER=*)
			test -z "$PAGER" && PAGER="${line#*=}"
			;;
		EQN=*)
			test -z "$EQN" && EQN="${line#*=}"
			;;
		NEQN=*)
			test -z "$NEQN" && NEQN="${line#*=}"
			;;
		TBL=*)
			test -z "$TBL" && TBL="${line#*=}"
			;;
		REFER=*)
			test -z "$REFER" && REFER="${line#*=}"
			;;
		VGRIND=*)
			test -z "$VGRIND" && VGRIND="${line#*=}"
			;;
		MACSET=*)
			test -z "$MACSET" && MACSET="${line#*=}"
			;;
		HTML1=*)
			test -z "$HTML1" && HTML1="${line#*=}"
			;;
		HTML2=*)
			test -z "$HTML2" && HTML2="${line#*=}"
			;;
		SGML=*)
			test -z "$SGML" && SGML="${line#*=}"
			;;
		esac
	done < "$DFLDIR/man"
fi

#
# Use defaults for values that are neither in the environment nor in
# the configuration file.
#
test -z "$MANPATH" && MANPATH=/usr/local/share/man:/usr/share/man
test -z "$DEFMANPATH" && DEFMANPATH=/usr/local/share/man:/usr/share/man
case "$MANPATH" in
*::*)
	MANPATH="${MANPATH%%::*}:$DEFMANPATH:${MANPATH#*::}"
esac
test -z "$TROFF" && TROFF=troff
test -z "$NROFF" && NROFF='nroff -Tlp'
test -z "$COL" && COL='col -x'
test -z "$PAGER" && PAGER=pg
test -z "$EQN" && EQN=eqn
test -z "$NEQN" && NEQN=neqn
test -z "$TBL" && TBL=tbl
test -z "$REFER" && REFER=refer
test -z "$VGRIND" && VGRIND=vgrind
test -z "$MACSET" && MACSET=-man
test -z "$HTML1" && {
	if (type w3m) >/dev/null 2>&1
	then
		HTML1='w3m -dump -t 8 -cols 66'
	elif test -x /usr/bin/html2ascii	# Open UNIX
	then
		HTML1='/usr/bin/html2ascii <'
	else
		HTML1=cat
	fi
}
test -z "$HTML2" && HTML2=
test -z "$SGML" && SGML=/usr/lib/sgml/sgml2roff	# Solaris

if test -n "$fflag" || test -n "$kflag" || test "$progname" = whatis ||
	test "$progname" = apropos
then
	if test -n "$aflag" || test -n "$debug" || test -n "$lflag" ||
		test -n "$tflag" || test -n "$section"
	then
		usage
	fi
	if test -n "$fflag" || test "$progname" = whatis
	then
		whatpropos whatis "$@"
	elif test -n "$kflag" || test "$progname" = apropos
	then
		whatpropos apropos "$@"
	fi
fi

if test -n "$debug"
then
	echo "MANPATH=$MANPATH"
	echo "TROFF=$TROFF"
	echo "NROFF=$NROFF"
	echo "COL=$COL"
	echo "TCAT=$TCAT"
	echo "PAGER=$PAGER"
	echo "EQN=$EQN"
	echo "NEQN=$NEQN"
	echo "TBL=$TBL"
	echo "REFER=$REFER"
	echo "VGRIND=$VGRIND"
	echo "MACSET=$MACSET"
	echo
fi

trap cleanup HUP INT QUIT TERM

if test "$progname" = catman
then
	docatman "$@"
fi

needpage=0
for name
do
	#
	# Traditional section specification, interpreted if -s was not given.
	#
	if test -z "$section"
	then
		case $name in
		[0123456789]?([abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ]))
			lowercase tradsection "$name"
			psection=$name
			needpage=1
			continue
			;;
		new|local|old|public)
			tradsection=${name%${name#?}}
			psection=$name
			needpage=1
			continue
		esac
	fi
	needpage=0
	gotcha=, textno=0 visited=:
	if test -n "$dflag"
	then
		format "$name" man "${name##*.}" "$name"
		manpath=
	else
		manpath=":$MANPATH:"
	fi
	while dir="${manpath%%:*}"; manpath="${manpath#*:}"
		test -n "$dir" || test -n "$manpath"
	do
		test -z "$dir" && continue
		case "$visited" in
		*:"$dir"*(/):*)	continue ;;
		esac
		visited="$visited$dir:"
		test -d "$dir" && cd "$dir" 2>/dev/null || {
			test -n "$specpath" &&
			echo "$dir is not accessible." >&2; continue;
		}
		test -n "$debug" && echo "mandir path = $dir"
		if test -n "$system"
		then
			test -d "$system" && cd "$system" 2>/dev/null ||
				continue
			dir="$dir/$system"
		fi
		MANSECTS=
		if test -n "$section"
		then
			MANSECTS="$section"
			psection=$section
		elif test -n "$tradsection"
		then
			MANSECTS="$tradsection"
		elif test -r man.cf
		then
			while IFS= read -r line
			do
				case "$line" in
				\#*|'')
					;;
				MANSECTS=*)
					MANSECTS="${line#*=}"
					;;
				esac
			done < man.cf
			test -n "$debug" -a -n "$MANSECTS" &&
				echo "search order = $MANSECTS"
		fi
		if test -n "$MANSECTS"
		then
			mansects=",$MANSECTS,"
			while sect="${mansects%%,*}"; mansects="${mansects#*,}"
				test -n "$sect" || test -n "$mansects"
			do
				test -z "$sect" && continue
				trydir "cat$sect" "man$sect" "html.$sect" \
					"sman$sect" "man$sect.Z"
				test -z "$aflag" -a "$gotcha" != , &&
					break 2
			done
		else
			trydir cat* man* html.* sman*
			test -z "$aflag" -a "$gotcha" != , && break
		fi
	done
	if test "$gotcha" = ,
	then	
		if test -n "$dflag"
		then
			echo "$name not found."
		elif test -n "$section"
		then
			echo "No entry for $name in section(s) $psection of the manual."
		elif test -n "$tradsection"
		then
			echo "No entry for $name in section $psection of the manual."
		else
			echo "No manual entry for $name."
		fi
	fi

	#
	# Results, if any, are now stored in temporary files. Display them.
	#
	cd "$oldpwd"
	if test -n "$texts"
	then
		if test -n "$debug"
		then
			echo "showing =$texts"
		elif test -n "$ontty" -a -z "$tflag"
		then
			$PAGER $texts
		else
			cat $texts
		fi
		rm -f $texts
		texts=
	fi
done

if test $needpage = 1
then
	echo "But what do you want from section $psection?" >&2
	exit 1
fi




