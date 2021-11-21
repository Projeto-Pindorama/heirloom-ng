all: cksum

cksum: cksum.o
	$(LD) $(LDFLAGS) cksum.o $(LCOMMON) $(LIBS) -o cksum

cksum.o: cksum.c
	$(CC) $(CFLAGS2) $(CPPFLAGS) $(XO5FL) $(LARGEF) -c cksum.c

install: all
	$(UCBINST) -c cksum $(ROOT)$(DEFBIN)/cksum
	$(STRIP) $(ROOT)$(DEFBIN)/cksum
	$(MANINST) -c -m 644 cksum.1 $(ROOT)$(MANDIR)/man1/cksum.1

clean:
	rm -f cksum cksum.o core log *~
