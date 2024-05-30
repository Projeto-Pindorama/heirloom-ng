# pick.sh - select arguments
#
# Copyright (C) 2024: Luiz Antônio Rangel (takusuman)
# SPDX-Licence-Identifier: Zlib

main() {
	case "`echo "x$@"`" in
		'x') # stdin
			while read input; do
				question "$input" || break
			done ;;
		*) # argv
			for input do
				question "$input" || break
			done ;;
	esac
	exit 0
}

question() {
	eprintf '%s ? ' "$1"
	read r
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
