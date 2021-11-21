all: mkfifo

mkfifo: mkfifo.o
	$(LD) $(LDFLAGS) mkfifo.o $(LCOMMON) $(LIBS) -o mkfifo

mkfifo.o: mkfifo.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(GNUFL) $(LARGEF) -c mkfifo.c

install: all
	$(UCBINST) -c mkfifo $(ROOT)$(DEFBIN)/mkfifo
	$(STRIP) $(ROOT)$(DEFBIN)/mkfifo
	$(MANINST) -c -m 644 mkfifo.1 $(ROOT)$(MANDIR)/man1/mkfifo.1

clean:
	rm -f mkfifo mkfifo.o core log *~
