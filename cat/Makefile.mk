all: cat

cat: cat.o
	$(LD) $(LDFLAGS) cat.o $(LCOMMON) $(LWCHAR) $(LIBS) -o cat

cat.o: cat.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(IWCHAR) $(ICOMMON) -c cat.c

install: all
	$(UCBINST) -c cat $(ROOT)$(DEFBIN)/cat
	$(STRIP) $(ROOT)$(DEFBIN)/cat
	$(MANINST) -c -m 644 cat.1 $(ROOT)$(MANDIR)/man1/cat.1

clean:
	rm -f cat cat.o core log *~
