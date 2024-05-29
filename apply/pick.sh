# pick.sh - select arguments
#
# Copyright (C) 2024: Luiz Antônio Rangel (takusuman)
# SPDX-Licence-Identifier: Zlib

main() {
	if [ "`echo "x$@"`" \= 'x' ]; then
		# stdin
		while read input; do
			question "$input" || break
		done
	else
		# argv
		for input do
			question "$input" || break
		done
	fi
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
