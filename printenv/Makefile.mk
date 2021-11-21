all: printenv

printenv: printenv.o
	$(LD) $(LDFLAGS) printenv.o $(LCOMMON) $(LWCHAR) $(LIBS) -o printenv

printenv.o: printenv.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(IWCHAR) $(ICOMMON) -c printenv.c

install: all
	$(UCBINST) -c printenv $(ROOT)$(DEFBIN)/printenv
	$(STRIP) $(ROOT)$(DEFBIN)/printenv
	$(MANINST) -c -m 644 printenv.1 $(ROOT)$(MANDIR)/man1/printenv.1

clean:
	rm -f printenv printenv.o core log *~
