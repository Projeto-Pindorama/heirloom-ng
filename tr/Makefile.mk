all: tr tr_sus tr_ucb

tr: tr.o
	$(LD) $(LDFLAGS) tr.o $(LCOMMON) $(LWCHAR) $(LIBS) -o tr

tr.o: tr.c
	$(CC) $(CFLAGS2) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(IWCHAR) $(ICOMMON) -c tr.c

tr_sus: tr_sus.o
	$(LD) $(LDFLAGS) tr_sus.o $(LCOMMON) $(LWCHAR) $(LIBS) -o tr_sus

tr_sus.o: tr.c
	$(CC) $(CFLAGS2) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(IWCHAR) $(ICOMMON) -DSUS -c tr.c -o tr_sus.o

tr_ucb: tr_ucb.o
	$(LD) $(LDFLAGS) tr_ucb.o $(LCOMMON) $(LWCHAR) $(LIBS) -o tr_ucb

tr_ucb.o: tr.c
	$(CC) $(CFLAGS2) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(IWCHAR) $(ICOMMON) -DUCB -c tr.c -o tr_ucb.o

install: all
	$(UCBINST) -c tr $(ROOT)$(SV3BIN)/tr
	$(STRIP) $(ROOT)$(SV3BIN)/tr
	$(UCBINST) -c tr_sus $(ROOT)$(SUSBIN)/tr
	$(STRIP) $(ROOT)$(SUSBIN)/tr
	$(UCBINST) -c tr_ucb $(ROOT)$(UCBBIN)/tr
	$(STRIP) $(ROOT)$(UCBBIN)/tr
	$(MANINST) -c -m 644 tr.1 $(ROOT)$(MANDIR)/man1/tr.1
	$(MANINST) -c -m 644 tr.1b $(ROOT)$(MANDIR)/man1b/tr.1b

clean:
	rm -f tr tr.o tr_sus tr_sus.o tr_ucb tr_ucb.o core log *~
