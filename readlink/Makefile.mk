all: readlink readlink_ucb

readlink: readlink.o
	$(LD) $(LDFLAGS) readlink.o $(LCOMMON) $(LIBS) -o readlink

readlink.o: readlink.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(ICOMMON) -c readlink.c

readlink_ucb: readlink_ucb.o
	$(LD) $(LDFLAGS) readlink_ucb.o $(LCOMMON) $(LIBS) -o readlink_ucb

readlink_ucb.o: readlink.c
	$(CC) $(CFLAGS) -DUCB $(CPPFLAGS) $(ICOMMON) -c readlink.c -o readlink_ucb.o

install: all
	$(UCBINST) -c readlink_ucb $(ROOT)$(UCBBIN)/readlink
	$(STRIP) $(ROOT)$(UCBBIN)/readlink
	$(UCBINST) -c readlink $(ROOT)$(DEFBIN)/readlink
	$(STRIP) $(ROOT)$(DEFBIN)/readlink
	$(MANINST) -c -m 644 readlink.1b $(ROOT)$(MANDIR)/man1/readlink.1b

clean:
	rm -f readlink readlink.o core log *~
