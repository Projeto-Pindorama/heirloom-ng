all: uname

uname: uname.o
	$(LD) $(LDFLAGS) uname.o $(LCOMMON) $(LIBS) -o uname

uname.o: uname.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c uname.c

install: all
	$(UCBINST) -c uname $(ROOT)$(DEFBIN)/uname
	$(STRIP) $(ROOT)$(DEFBIN)/uname
	$(MANINST) -c -m 644 uname.1 $(ROOT)$(MANDIR)/man1/uname.1

clean:
	rm -f uname uname.o core log *~
