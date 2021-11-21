all: tee

tee: tee.o
	$(LD) $(LDFLAGS) tee.o $(LCOMMON) $(LIBS) -o tee

tee.o: tee.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) -c tee.c

install: all
	$(UCBINST) -c tee $(ROOT)$(DEFBIN)/tee
	$(STRIP) $(ROOT)$(DEFBIN)/tee
	$(MANINST) -c -m 644 tee.1 $(ROOT)$(MANDIR)/man1/tee.1

clean:
	rm -f tee tee.o core log *~
