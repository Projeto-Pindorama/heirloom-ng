all: tail

tail: tail.o
	$(LD) $(LDFLAGS) tail.o $(LCOMMON) $(LIBS) -o tail

tail.o: tail.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO6FL) $(LARGEF) $(ICOMMON) -c tail.c

install: all
	$(UCBINST) -c tail $(ROOT)$(DEFBIN)/tail
	$(STRIP) $(ROOT)$(DEFBIN)/tail
	$(MANINST) -c -m 644 tail.1 $(ROOT)$(MANDIR)/man1/tail.1

clean:
	rm -f tail tail.o core log *~
