all: ln_ucb

ln_ucb: ln_ucb.o
	$(LD) $(LDFLAGS) ln_ucb.o $(LCOMMON) $(LIBS) -o ln_ucb

ln_ucb.o: ln_ucb.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) -c ln_ucb.c

install: all
	$(UCBINST) -c ln_ucb $(ROOT)$(UCBBIN)/ln
	$(STRIP) $(ROOT)$(UCBBIN)/ln
	$(MANINST) -c -m 644 ln.1b $(ROOT)$(MANDIR)/man1b/ln.1b

clean:
	rm -f ln_ucb ln_ucb.o core log *~
