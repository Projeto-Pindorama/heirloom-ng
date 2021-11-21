all: nice

nice: nice.o
	$(LD) $(LDFLAGS) nice.o $(LCOMMON) $(LIBS) -o nice

nice.o: nice.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) -c nice.c

install: all
	$(UCBINST) -c nice $(ROOT)$(DEFBIN)/nice
	$(STRIP) $(ROOT)$(DEFBIN)/nice
	$(MANINST) -c -m 644 nice.1 $(ROOT)$(MANDIR)/man1/nice.1

clean:
	rm -f nice nice.o core log *~
