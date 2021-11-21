all: bc

bc: bc.o
	$(LD) $(LDFLAGS) bc.o $(LCOMMON) $(LIBS) -o bc

bc.o: bc.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) -DDC='"$(SV3BIN)/dc"' -DLIBB='"$(DEFLIB)/lib.b"' -c bc.c

bc.c: bc.y
	$(YACC) $<
	sed -f yyval.sed <y.tab.c >$@
	rm y.tab.c

install: all
	$(UCBINST) -c bc $(ROOT)$(DEFBIN)/bc
	$(STRIP) $(ROOT)$(DEFBIN)/bc
	$(MANINST) -c -m 644 bc.1 $(ROOT)$(MANDIR)/man1/bc.1
	$(UCBINST) -c -m 644 lib.b $(ROOT)$(DEFLIB)/lib.b

clean:
	rm -f bc bc.o bc.c core log *~
