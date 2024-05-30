# pick.sh - select arguments
#
# Copyright (C) 2024: Luiz Antônio Rangel (takusuman)
# SPDX-Licence-Identifier: Zlib

main() {
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
	for input in "$@"; do
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
		'y') echo "$1" ;;
		'q') ec=1 ;;
		*) ec=0 ;;
	esac
	unset input
	return $ec
}

eprintf() {
	printf 1>&2 "$@"
}

main "$@"
