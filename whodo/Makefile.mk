all: whodo

whodo: whodo.o
	$(LD) $(LDFLAGS) whodo.o $(LCOMMON) $(LWCHAR) $(LKVM) $(LIBS) -o whodo

whodo.o: whodo.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(IWCHAR) $(ICOMMON) -c whodo.c

install: all
	$(UCBINST) -c whodo $(ROOT)$(DEFBIN)/whodo
	$(STRIP) $(ROOT)$(DEFBIN)/whodo
	rm -f $(ROOT)$(DEFBIN)/w $(ROOT)$(DEFBIN)/uptime
	$(LNS) whodo $(ROOT)$(DEFBIN)/w
	$(LNS) whodo $(ROOT)$(DEFBIN)/uptime
	$(MANINST) -c -m 644 whodo.1 $(ROOT)$(MANDIR)/man1/whodo.1
	$(MANINST) -c -m 644 w.1 $(ROOT)$(MANDIR)/man1/w.1
	$(MANINST) -c -m 644 uptime.1 $(ROOT)$(MANDIR)/man1/uptime.1

clean:
	rm -f whodo whodo.o core log *~
