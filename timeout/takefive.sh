#!/bin/sh
# takefive.sh: test script for timeout(1), takes
# minutes as argument.
# Better if compiled with shcomp(1), since it
# gives us an independent P.ID.

main() {
	for SIG in ABRT ALRM HUP INT TERM; do
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

handle_signal() {
	printf 'Caught %s\n' $1
}

main $1
