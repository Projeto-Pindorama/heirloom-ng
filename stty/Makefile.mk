all: stty stty_ucb

stty: stty.o
	$(LD) $(LDFLAGS) stty.o $(LCOMMON) $(LWCHAR) $(LIBS) -o stty

stty.o: stty.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(GNUFL) $(IWCHAR) $(ICOMMON) -c stty.c

stty_ucb: stty_ucb.o
	$(LD) $(LDFLAGS) stty_ucb.o $(LCOMMON) $(LWCHAR) $(LIBS) -o stty_ucb

stty_ucb.o: stty.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(GNUFL) $(IWCHAR) $(ICOMMON) -DUCB -c stty.c -o stty_ucb.o

install: all
	$(UCBINST) -c stty $(ROOT)$(DEFBIN)/stty
	$(STRIP) $(ROOT)$(DEFBIN)/stty
	rm -f $(ROOT)$(DEFBIN)/STTY
	$(LNS) stty $(ROOT)$(DEFBIN)/STTY
	$(UCBINST) -c stty_ucb $(ROOT)$(UCBBIN)/stty
	$(STRIP) $(ROOT)$(UCBBIN)/stty
	$(MANINST) -c -m 644 stty.1 $(ROOT)$(MANDIR)/man1/stty.1
	$(MANINST) -c -m 644 stty.1b $(ROOT)$(MANDIR)/man1b/stty.1b

clean:
	rm -f stty stty.o stty_ucb stty_ucb.o core log *~
