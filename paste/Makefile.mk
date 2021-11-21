all: paste

paste: paste.o
	$(LD) $(LDFLAGS) paste.o $(LCOMMON) $(LWCHAR) $(LIBS) -o paste

paste.o: paste.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(ICOMMON) $(IWCHAR) $(LARGEF) -c paste.c

install: all
	$(UCBINST) -c paste $(ROOT)$(DEFBIN)/paste
	$(STRIP) $(ROOT)$(DEFBIN)/paste
	$(MANINST) -c -m 644 paste.1 $(ROOT)$(MANDIR)/man1/paste.1

clean:
	rm -f paste paste.o core log *~
