all: true false

true: truther.c
	$(CC) $(CFLAGS) $(CPPFLAGS) truther.c -DTRUE -o true

false: truther.c
	$(CC) $(CFLAGS) $(CPPFLAGS) truther.c -DFALSE -o false
	
install: all
	$(UCBINST) -c true $(ROOT)$(DEFBIN)/true
	$(UCBINST) -c false $(ROOT)$(DEFBIN)/false
	$(MANINST) -c -m 644 true.1 $(ROOT)$(MANDIR)/man1/true.1
	rm -f $(ROOT)$(MANDIR)/man1/false.1
	$(LNS) true.1 $(ROOT)$(MANDIR)/man1/false.1

clean:
	rm -f true false *~
