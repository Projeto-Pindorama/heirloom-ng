all: seq

seq: seq.o
	$(LD) $(LDFLAGS) seq.o $(LCOMMON) $(LIBS) -o seq

seq.o: seq.c
	$(CC) -std=c99 $(CFLAGSU) $(CPPFLAGS) $(ICOMMON) -c seq.c

install: all
	$(UCBINST) -c seq $(ROOT)$(DEFBIN)/seq
	$(STRIP) $(ROOT)$(DEFBIN)/seq
	$(MANINST) -c -m 644 seq.1 $(ROOT)$(MANDIR)/man1/seq.1

clean:
	rm -f seq seq.o core log *~
