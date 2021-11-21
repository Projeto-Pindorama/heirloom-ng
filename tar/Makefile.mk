all: tar

tar: tar.o
	$(LD) $(LDFLAGS) tar.o $(LCOMMON) $(LWCHAR) $(LIBS) -o tar

tar.o: tar.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(GNUFL) $(LARGEF) $(IWCHAR) $(ICOMMON) -DTARDFL='"$(DFLDIR)/tar"' -DSHELL='"$(SHELL)"' -DSV3BIN='"$(SV3BIN)"' -DDEFBIN='"$(DEFBIN)"' -c tar.c

install: all
	$(UCBINST) -c tar $(ROOT)$(DEFBIN)/tar
	$(STRIP) $(ROOT)$(DEFBIN)/tar
	test -r $(ROOT)$(DFLDIR)/tar || $(UCBINST) -c -m 644 tar.dfl $(ROOT)$(DFLDIR)/tar
	$(MANINST) -c -m 644 tar.1 $(ROOT)$(MANDIR)/man1/tar.1

clean:
	rm -f tar tar.o core log *~
