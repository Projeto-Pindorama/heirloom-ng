#!/sbin/sh

# Sccsid @(#)genintro.sh	1.6 (gritter) 6/24/05

LC_ALL=C export LC_ALL

T=/tmp/genintro.$$
trap "rm -f $T; exit" 0 1 2 3 13 15

test $# -gt 0 && cat "$@"

cat <<\!
.SS "List of commands"
.TS
l1 l1 l.
Name	Appears on Page	Description
!

if test -x nawk/awk
then
	awk=nawk/awk
else
	awk=nawk
fi

$awk '
/^.TH / {
	page = tolower($2)
	sect = $3
}
/^.SH NAME$/ {
	getline
	comd = substr($0, 1, index($0, " \\- ") - 1)
	desc = substr($0, index($0, " \\- ") + 4)
	if (sect ~ /^[18]/ && comd != "intro") {
		while (comd ~ /, /) {
			c1 = substr(comd, 1, index(comd, ", ") - 1)
			comd = substr(comd, index(comd, ", ") + 2)
			printf("%s\t%s(%s)\t%s\n", c1, page, sect, desc)
		}
		printf("%s\t%s(%s)\t%s\n", comd, page, sect, desc)
	} else
		printf("%s(%s)\t%s\n", page, sect, desc) >"'$T'"
}' `ls */*.[0-9]* | sed '/^attic/ d'` | sort -u

cat <<\!
.TE
.SS "Other manual entries"
.TS
l1 l.
Page	Description
!

sort -u $T

cat <<\!
.TE
!
