all: what

what: what.o
	$(LD) $(LDFLAGS) what.o $(LCOMMON) $(LIBS) -o what

what.o: what.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) -c what.c

install: all
	$(UCBINST) -c what $(ROOT)$(DEFBIN)/what
	$(STRIP) $(ROOT)$(DEFBIN)/what
	$(MANINST) -c -m 644 what.1 $(ROOT)$(MANDIR)/man1/what.1

clean:
	rm -f what what.o core log *~
