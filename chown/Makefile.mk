all: chown chown_ucb

chown: chown.o
	$(LD) $(LDFLAGS) chown.o $(LCOMMON) $(LIBS) -o chown

chown.o: chown.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) -c chown.c

chown_ucb: chown_ucb.o
	$(LD) $(LDFLAGS) chown_ucb.o $(LCOMMON) $(LIBS) -o chown_ucb

chown_ucb.o: chown.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) -DUCB -c chown.c -o chown_ucb.o

install: all
	$(UCBINST) -c chown $(ROOT)$(DEFBIN)/chown
	$(STRIP) $(ROOT)$(DEFBIN)/chown
	rm -f $(ROOT)$(DEFBIN)/chgrp
	$(LNS) chown $(ROOT)$(DEFBIN)/chgrp
	$(UCBINST) -c chown_ucb $(ROOT)$(UCBBIN)/chown
	$(STRIP) $(ROOT)$(UCBBIN)/chown
	$(MANINST) -c -m 644 chown.1 $(ROOT)$(MANDIR)/man1/chown.1
	rm -f $(ROOT)$(MANDIR)/man1/chgrp.1
	$(LNS) chown.1 $(ROOT)$(MANDIR)/man1/chgrp.1
	$(MANINST) -c -m 644 chown.1b $(ROOT)$(MANDIR)/man1b/chown.1b

clean:
	rm -f chown chown.o chown_ucb chown_ucb.o core log *~
