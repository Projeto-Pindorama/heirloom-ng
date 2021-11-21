#
# Sccsid @(#)yyval.sed	1.3 (gritter) 4/27/04
#
# bison has a yacc-compatible yyval, but it is a local variable inside
# yyparse(). Making the variable global is necessary to make bc work
# with a bison-generated parser.
1,2 {
	/Bison/ {
	:look
		/YYSTYPE/ {
			a\
			YYSTYPE yyval;
		:repl
			s/^[ 	]*YYSTYPE[ 	]*yyval;//
			n
			t
			b repl
		}
		n
		b look
	}
}
