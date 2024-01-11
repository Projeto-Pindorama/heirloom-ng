#!/bin/sh
# takefive.sh: test script for timeout(1), takes
# minutes as argument.
# Better if compiled with shcomp(1), since it
# gives us an independent P.ID.

main() {
	take=${1:-5}
	printf 1>&2 'Sleeping %d seconds...\n' $take
	sleep $take
	return 3 
}; main $1
