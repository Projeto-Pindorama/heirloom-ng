all: install_ucb

install_ucb: install_ucb.o
	$(LD) $(LDFLAGS) install_ucb.o $(LCOMMON) $(LIBS) -o install_ucb

install_ucb.o: install_ucb.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LARGEF) $(XO6FL) -c install_ucb.c

install: all
	rm -f $(ROOT)$(UCBBIN)/install
	cp install_ucb $(ROOT)$(UCBBIN)/install
	$(STRIP) $(ROOT)$(UCBBIN)/install
	chmod a+x $(ROOT)$(UCBBIN)/install
	$(MANINST) -c -m 644 install.1b $(ROOT)$(MANDIR)/man1b/install.1b

clean:
	rm -f install_ucb install_ucb.o core log *~
