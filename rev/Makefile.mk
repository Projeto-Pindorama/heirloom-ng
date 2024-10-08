all: rev

rev: rev.o
	$(LD) $(LDFLAGS) rev.o $(LCOMMON) $(LWCHAR) $(LIBS) -o rev

rev.o: rev.c
	$(CC) $(CFLAGSU) $(CPPFLAGS) $(ICOMMON) $(IWCHAR) -c rev.c 

install: all
	$(UCBINST) -c rev $(ROOT)$(DEFBIN)/rev
	$(STRIP) $(ROOT)$(DEFBIN)/rev
	$(MANINST) -c -m 644 rev.1 $(ROOT)$(MANDIR)/man1/rev.1

clean:
	rm -f rev rev.o core log *~
