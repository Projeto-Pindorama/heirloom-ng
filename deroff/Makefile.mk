all: deroff deroff_ucb

deroff: deroff.o
	$(LD) $(LDFLAGS) deroff.o $(LCOMMON) $(LIBS) -o deroff

deroff.o: deroff.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) -c deroff.c

deroff_ucb: deroff_ucb.o
	$(LD) $(LDFLAGS) deroff_ucb.o $(LCOMMON) $(LIBS) -o deroff_ucb

deroff_ucb.o: deroff_ucb.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) -c deroff_ucb.c

install: all
	$(UCBINST) -c deroff $(ROOT)$(DEFBIN)/deroff
	$(STRIP) $(ROOT)$(DEFBIN)/deroff
	$(MANINST) -c -m 644 deroff.1 $(ROOT)$(MANDIR)/man1/deroff.1
	$(UCBINST) -c deroff_ucb $(ROOT)$(UCBBIN)/deroff
	$(STRIP) $(ROOT)$(UCBBIN)/deroff
	$(MANINST) -c -m 644 deroff.1b $(ROOT)$(MANDIR)/man1b/deroff.1b

clean:
	rm -f deroff deroff.o deroff_ucb deroff_ucb.o core log *~
