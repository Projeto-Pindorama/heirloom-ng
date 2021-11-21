all: hostname

hostname: hostname.o
	$(LD) $(LDFLAGS) hostname.o $(LCOMMON) $(LWCHAR) $(LIBS) -o hostname

hostname.o: hostname.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(GNUFL) $(LARGEF) $(IWCHAR) $(ICOMMON) -c hostname.c

install: all
	$(UCBINST) -c hostname $(ROOT)$(DEFBIN)/hostname
	$(STRIP) $(ROOT)$(DEFBIN)/hostname
	$(MANINST) -c -m 644 hostname.1 $(ROOT)$(MANDIR)/man1/hostname.1

clean:
	rm -f hostname hostname.o core log *~
