all: tapecntl mt tape

tapecntl: tapecntl.o
	$(LD) $(LDFLAGS) tapecntl.o $(LCOMMON) $(LIBS) -o tapecntl

tapecntl.o: tapecntl.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) -DTAPEDFL='"$(DFLDIR)/tape"' -c tapecntl.c

tape mt: tapecntl
	rm -f $@
	$(LNS) tapecntl $@

install: all
	$(UCBINST) -c tapecntl $(ROOT)$(DEFBIN)/tapecntl
	$(STRIP) $(ROOT)$(DEFBIN)/tapecntl
	rm -f $(ROOT)$(DEFBIN)/mt
	$(LNS) tapecntl $(ROOT)$(DEFBIN)/mt
	rm -f $(ROOT)$(DEFBIN)/tape
	$(LNS) tapecntl $(ROOT)$(DEFBIN)/tape
	$(MANINST) -c -m 644 mt.1 $(ROOT)$(MANDIR)/man1/mt.1
	$(MANINST) -c -m 644 tape.1 $(ROOT)$(MANDIR)/man1/tape.1
	$(MANINST) -c -m 644 tapecntl.1 $(ROOT)$(MANDIR)/man1/tapecntl.1

clean:
	rm -f mt tape tapecntl tapecntl.o core log *~
