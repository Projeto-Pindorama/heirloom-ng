# pick.sh - select arguments
#
# Copyright (C) 2024: Luiz Antônio Rangel (takusuman)
# SPDX-Licence-Identifier: Zlib

main() {
	while getopts ':f' c; do
		case $c in
			f) # sets echo as boilerplate
			   # for printq.
				echo() { printq "$@"; }
				break ;;
			\?) # ignore
				break ;;
		esac
	done
	shift `expr $OPTIND - 1`

	case "`echo "x$@"`" in
		'x') # stdin
			while read -r l; do
				# An "arrayoid", so we can
				# cache std.in. into "argv".
				set -- "$@" "$l"
			done
			break ;;
		*) # argv
			break ;;
	esac
	for input do
		question "$input" || break
	done
	exit 0
}

question() {
	eprintf '%s ? ' "$1"

	# Small hack, so we can get
	# input from the user without
	# having headaches with std.in.
	read r </dev/tty
	case "$r" in
		'y') echo "$1"
			break ;;
		'q') ec=1
			break ;;
		*) ec=0
			break ;;
	esac
	unset input r
	return $ec
}

# References:
# https://github.com/bminor/bash/blob/bash-5.2/builtins/printf.def#L595-L644
# https://github.com/digitalocean/gnulib/blob/master/lib/quotearg.c#L255-L772
# https://github.com/ksh93/ksh/blob/dev/src/cmd/ksh93/sh/string.c#L453-L660
printq() {
	printf '%s' "$@" \
	| nawk -v RS='' '{
		gsub("\n", "\\n");
		gsub("\r", "\\r");
		gsub("\t", "\\t");
		gsub("'\''", "\\\'\''");
	} 1'
}

eprintf() {
	printf 1>&2 "$@"
}

main "$@"
