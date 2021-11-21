all: du du_sus du_ucb

du: du.o
	$(LD) $(LDFLAGS) du.o $(LCOMMON) $(LWCHAR) $(LIBS) -o du

du.o: du.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(ICOMMON) -c du.c

du_sus: du_sus.o
	$(LD) $(LDFLAGS) du_sus.o $(LCOMMON) $(LWCHAR) $(LIBS) -o du_sus

du_sus.o: du.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(ICOMMON) -DSUS -c du.c -o du_sus.o

du_ucb: du_ucb.o
	$(LD) $(LDFLAGS) du_ucb.o $(LCOMMON) $(LWCHAR) $(LIBS) -o du_ucb

du_ucb.o: du.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(ICOMMON) -DUCB -c du.c -o du_ucb.o

install:
	$(UCBINST) -c du $(ROOT)$(SV3BIN)/du
	$(STRIP) $(ROOT)$(SV3BIN)/du
	$(UCBINST) -c du_sus $(ROOT)$(SUSBIN)/du
	$(STRIP) $(ROOT)$(SUSBIN)/du
	$(UCBINST) -c du_ucb $(ROOT)$(UCBBIN)/du
	$(STRIP) $(ROOT)$(UCBBIN)/du
	$(MANINST) -c -m 644 du.1 $(ROOT)$(MANDIR)/man1/du.1
	$(MANINST) -c -m 644 du.1b $(ROOT)$(MANDIR)/man1b/du.1b

clean:
	rm -f du du.o du_sus du_sus.o du_ucb du_ucb.o core log *~
