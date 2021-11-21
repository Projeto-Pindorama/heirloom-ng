all: bdiff

bdiff: bdiff.o
	$(LD) $(LDFLAGS) bdiff.o $(LCOMMON) $(LWCHAR) $(LIBS) -o bdiff

bdiff.o: bdiff.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(GNUFL) $(LARGEF) $(IWCHAR) $(ICOMMON) -DDIFF='"$(DEFBIN)/diff"' -c bdiff.c

install: all
	$(UCBINST) -c bdiff $(ROOT)$(DEFBIN)/bdiff
	$(STRIP) $(ROOT)$(DEFBIN)/bdiff
	$(MANINST) -c -m 644 bdiff.1 $(ROOT)$(MANDIR)/man1/bdiff.1

clean:
	rm -f bdiff bdiff.o core log *~

bdiff.o: bdiff.c fatal.h ../libcommon/sigset.h
