# Sccsid @(#)mansubst.sh	1.5 (gritter) 8/30/03

# Write the maximum values into the manual page.

tmp=ms$$.c

trap "rm -f $tmp; exit" 0 1 2 13 15

cat >$tmp <<!
#include <float.h>
#include "config.h"
#ifdef	USE_LONG_DOUBLE
LDBL_MANT_DIG
#else
DBL_MANT_DIG
#endif
!

mant_dig=`$cc -E $tmp | sed '/^#/ d; s/^[ 	]*//; /^$/ d; /^[^0-9]/ d'`

num=`echo "2 ^ $mant_dig" | bc`
flt=`awk </dev/null "BEGIN {
	printf(\"%.2g\n\", $num)
}"`
exp=`echo $flt | sed 's/\.0//; s/e.*//'`
mnt=`echo $flt | sed 's/.*e+//'`

sed "
	s/@@MANT_DIG@@/$mant_dig/g
	s/@@EXP@@/$exp/g
	s/@@MNT@@/$mnt/g
"
