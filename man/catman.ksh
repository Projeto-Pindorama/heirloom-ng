
#
# Routines for catman command only.
#
#	Sccsid catman.ksh	1.2 (gritter) 8/10/03
#

#
# Add windex line for a page. $1 is the entry in mandir, $2 is the possibly
# uncompressed file to read from, $3 is the manual section.
#
windexadd() {
	w_name="${1##*/}"
	case "$3" in
	[0-9])
		w_name="${w_name%.$3*}"
		;;
	*)
		w_name="${w_name%.$3}"
		;;
	esac
	w_page= w_desc=
	while IFS= read -r w_line
	do
		case "$w_line" in
		.SH+( )NAME*( )|.SH+( )\"NAME\"*( ))
			while IFS= read -r w_line
			do
				case "$w_line" in
				[.\']*|'')
					;;
				*)
					break
					;;
				esac
			done
			case "$w_line" in
			+([! ]),*\ \\-\ *)
				test -z "$w_page" && w_page="${w_line%%,*}"
				w_desc="${w_line#*\\-+( )}"
				;;
			+([! ]),*\ -\ *)
				test -z "$w_page" && w_page="${w_line%%,*}"
				w_desc="${w_line#*-+( )}"
				;;
			*\ \\-\ *)
				test -z "$w_page" &&
					w_page="${w_line%%+( )\\- *}"
				w_desc="${w_line#*\\-+( )}"
				;;
			*\ -\ *)
				test -z "$w_page" &&
					w_page="${w_line%%+( )- *}"
				w_desc="${w_line#*-+( )}"
				;;
			*)
				return
				;;
			esac
			# collect broken continued .SH NAME descriptions
			while IFS= read -r w_line
			do
				case "$w_line" in
				.S?*|.?P*|*\ \\-\ *)
					break
					;;
				.+([A-Z][a-z])+( )*)
					w_desc="${w_line#.+([A-Z][a-z])+( )}"
					;;
				[.\']*|'')
					;;
				*)
					w_desc="$w_desc $w_line"
					;;
				esac
			done
			break
			;;
		.Nm+( )*)
			if test -z "$w_page"
			then
				w_page="${w_line#.Nm+( )}"
				case "$w_page" in
				**([ 	]),*)
					w_page="${w_page%%*([ 	]),*}"
				esac
			fi
			;;
		.Nd+( )*)
			w_desc="${w_line#.Nd+( )}"
			# collect broken continued .Nd descriptions
			while IFS= read -r w_line
			do
				case "$w_line" in
				.N*([A-Z][a-z])+( )*|.Sh+( )*)
					break
					;;
				.+([A-Z][a-z])+( )*)
					w_desc="${w_line#.+([A-Z][a-z])+( )}"
					;;
				[.\']*|'')
					;;
				*)
					w_desc="$w_desc $w_line"
					;;
				esac
			done
			break
			;;
		esac
	done < "$2"
	if test -z "$w_name" || test -z "$w_page" || test -z "$w_desc"
	then
		return
	fi
	if test "${#w_name}" -ge 8
	then
		w_sp1="	"
	else
		w_sp1="		"
	fi
	if test $((${#w_page} + ${#3} + 3)) -ge 8
	then
		w_sp2="	"
	else
		w_sp2="		"
	fi
	case "$w_desc" in
	\"+(?)\")
		w_desc="${w_desc#?}"
		w_desc="${w_desc%?}"
	esac
	w_str="$w_name$w_sp1$w_page ($3)$w_sp2- $w_desc"
	if test -z "$debug"
	then
		$printr "$w_str" >&5
	fi
}

#
# Do catman for a single man subdirectory.
#
catdir() {
	c_sect="${1#man}"
	if test -z "$wflag" && test ! -d "cat$c_sect"
	then
		if test -z "$debug"
		then
			mkdir "$dir/cat$c_sect" || return
		else
			echo "mkdir $dir/cat$c_sect"
		fi
	fi
	for c_file in "$1"/*."$c_sect"*
	do
		format "$c_file" man "$c_sect" "$c_file"
	done
}

docatman() {
	manpath=":$MANPATH:"
	while dir="${manpath%%:*}"; manpath="${manpath#*:}"
		test -n "$dir" || test -n "$manpath"
	do
		test -z "$dir" && continue
		cd "$dir" 2>/dev/null || test -z "$specpath" ||
			{ echo "$dir is not accessible." >&2; continue; }
		test -n "$debug" && echo "Building cat pages for mandir = $dir"
		if test -n "$system"
		then
			cd "$system" 2>/dev/null || continue
			dir="$dir/$system"
		fi
		if test -z "$nflag" && test -z "$debug"
		then
			exec 5> /tmp/mw$$ || continue
		fi
		if test $# -gt 0
		then
			for sect
			do
				test -d "man$sect" && catdir "man$sect"
			done
		else
			for subdir in man*
			do
				test -d "$subdir" && catdir "$subdir"
			done
		fi
		if test -z "$nflag" && test -z "$debug"
		then
			exec 5>&-
			sed -e 's/\\&//g
				s/\\f.//g
				s/\\f(..//g
				s/\\s[-+]*[0-9]*//g
				s/\\-/-/g
				s/\\[| ^]/ /g
				s/\\e/\\/g
				' /tmp/mw$$ > /tmp/mi$$
			if test $# -gt 0
			then
				cut=
				for sect
				do
				  cut="$cut|([ 	]+\($sect\)[ 	]+-[ 	]+)"
				done
				cut="${cut#|}"
				egrep -v "$cut" windex > /tmp/mj$$
				sort /tmp/mi$$ /tmp/mj$$ > windex
				rm /tmp/mi$$ /tmp/mj$$
			else
				sort /tmp/mi$$ > windex
				rm /tmp/mi$$
			fi
		fi
	done
	test -f /tmp/mw$$ && rm /tmp/mw$$
	exit
}
