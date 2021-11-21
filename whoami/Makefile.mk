all: whoami

whoami: whoami.o
	$(LD) $(LDFLAGS) whoami.o $(LCOMMON) $(LWCHAR) $(LIBS) -o whoami

whoami.o: whoami.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(IWCHAR) $(ICOMMON) -c whoami.c

install: all
	$(UCBINST) -c whoami $(ROOT)$(DEFBIN)/whoami
	$(STRIP) $(ROOT)$(DEFBIN)/whoami
	$(MANINST) -c -m 644 whoami.1 $(ROOT)$(MANDIR)/man1/whoami.1

clean:
	rm -f whoami whoami.o core log *~
