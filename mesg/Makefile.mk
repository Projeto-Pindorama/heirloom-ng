all: mesg

mesg: mesg.o
	$(LD) $(LDFLAGS) mesg.o $(LCOMMON) $(LWCHAR) $(LIBS) -o mesg

mesg.o: mesg.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(GNUFL) $(LARGEF) $(IWCHAR) $(ICOMMON) -c mesg.c

install: all
	$(UCBINST) -c mesg $(ROOT)$(DEFBIN)/mesg
	$(STRIP) $(ROOT)$(DEFBIN)/mesg
	$(MANINST) -c -m 644 mesg.1 $(ROOT)$(MANDIR)/man1/mesg.1

clean:
	rm -f mesg mesg.o core log *~
