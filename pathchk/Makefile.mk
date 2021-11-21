all: pathchk

pathchk: pathchk.o
	$(LD) $(LDFLAGS) pathchk.o $(LCOMMON) $(LWCHAR) $(LIBS) -o pathchk

pathchk.o: pathchk.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(GNUFL) $(LARGEF) $(IWCHAR) $(ICOMMON) -c pathchk.c

install: all
	$(UCBINST) -c pathchk $(ROOT)$(DEFBIN)/pathchk
	$(STRIP) $(ROOT)$(DEFBIN)/pathchk
	$(MANINST) -c -m 644 pathchk.1 $(ROOT)$(MANDIR)/man1/pathchk.1

clean:
	rm -f pathchk pathchk.o core log *~
