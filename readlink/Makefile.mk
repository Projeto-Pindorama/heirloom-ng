all: readlink

readlink: readlink.o
	$(LD) $(LDFLAGS) readlink.o $(LCOMMON) $(LIBS) -o readlink

readlink.o: readlink.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(ICOMMON) -c readlink.c

install: all
	$(UCBINST) -c readlink $(ROOT)$(UCBBIN)/readlink
	$(STRIP) $(ROOT)$(UCBBIN)/readlink
	$(MANINST) -c -m 644 readlink.1b $(ROOT)$(MANDIR)/man1/readlink.1b

clean:
	rm -f readlink readlink.o core log *~
