all: dirname

dirname: dirname.o
	$(LD) $(LDFLAGS) dirname.o $(LCOMMON) $(LIBS) -o dirname

dirname.o: dirname.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) -c dirname.c

install: all
	$(UCBINST) -c dirname $(ROOT)$(DEFBIN)/dirname
	$(STRIP) $(ROOT)$(DEFBIN)/dirname
	$(MANINST) -c -m 644 dirname.1 $(ROOT)$(MANDIR)/man1/dirname.1

clean:
	rm -f dirname dirname.o core log *~
