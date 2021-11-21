all: ul

ul: ul.o
	$(LD) $(LDFLAGS) ul.o $(LCURS) $(LCOMMON) $(LWCHAR) $(LIBS) -o ul

ul.o: ul.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(IWCHAR) $(ICOMMON) -c ul.c

install: all
	$(UCBINST) -c ul $(ROOT)$(DEFBIN)/ul
	$(STRIP) $(ROOT)$(DEFBIN)/ul
	$(MANINST) -c -m 644 ul.1 $(ROOT)$(MANDIR)/man1/ul.1

clean:
	rm -f ul ul.o core log *~
