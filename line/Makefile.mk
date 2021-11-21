all: line

line: line.o
	$(LD) $(LDFLAGS) line.o $(LCOMMON) $(LIBS) -o line

line.o: line.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) -c line.c

install: all
	$(UCBINST) -c line $(ROOT)$(DEFBIN)/line
	$(STRIP) $(ROOT)$(DEFBIN)/line
	$(MANINST) -c -m 644 line.1 $(ROOT)$(MANDIR)/man1/line.1

clean:
	rm -f line line.o core log *~
