#!/bin/sh
# takefive.sh: test script for timeout(1), takes
# seconds as argument.
# Copyright (c) 2023, 2024 Luiz Antônio Rangel.
#
# This script is public domain, no more, no less.

main() {
	for SIG in ABRT ALRM CHLD HUP INT TERM; do
	eval "trap 'handle_signal $SIG' $SIG"
	done
	take=${1:-5}
	printf 1>&2 'Sleeping %d seconds...\n' $take
	while [ $take -gt 0 ]; do
		take=$(($take - 1))
		printf '%d... ' $take
		sleep 1
	done
	sleep $take
	return 3 
}

handle_signal() { # function input: string signal
	s=$1
	printf 'Caught %s\n' "$1"
}

main $1
