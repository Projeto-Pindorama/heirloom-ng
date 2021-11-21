all: tty

tty: tty.o
	$(LD) $(LDFLAGS) tty.o $(LCOMMON) $(LIBS) -o tty

tty.o: tty.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) -c tty.c

install: all
	$(UCBINST) -c tty $(ROOT)$(DEFBIN)/tty
	$(STRIP) $(ROOT)$(DEFBIN)/tty
	$(MANINST) -c -m 644 tty.1 $(ROOT)$(MANDIR)/man1/tty.1

clean:
	rm -f tty tty.o core log *~
