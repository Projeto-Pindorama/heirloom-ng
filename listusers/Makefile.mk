all: listusers

listusers: listusers.o
	$(LD) $(LDFLAGS) listusers.o $(LCOMMON) $(LIBS) -o listusers

listusers.o: listusers.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) -c listusers.c

install: all
	$(UCBINST) -c listusers $(ROOT)$(DEFBIN)/listusers
	$(STRIP) $(ROOT)$(DEFBIN)/listusers
	$(MANINST) -c -m 644 listusers.1 $(ROOT)$(MANDIR)/man1/listusers.1

clean:
	rm -f listusers listusers.o core log *~
