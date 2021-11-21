#
# Sccsid @(#)rerule.sed	1.1 (gritter) 2/6/05
#
# Change the awk grammar for POSIX.1-2001 such that expressions like
# "print /x/ / 2" work.
#
/^[ 	]*| re[ 	]*$/d

/^term:/ {
:loop
	/^[ 	]*;[ 	]*$/ {
		c\
	| re\
	;
		b
	}
	n
	b loop
}
