all: watch

watch: watch.o
	$(LD) $(LDFLAGS) watch.o $(LCURS) $(LCOMMON) $(LIBS) -o watch

watch.o: watch.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(ICOMMON) -c watch.c

install: all
	$(UCBINST) -c watch $(ROOT)$(DEFBIN)/watch
	$(STRIP) $(ROOT)$(DEFBIN)/watch
#	$(MANINST) -c -m 644 watch.1 $(ROOT)$(MANDIR)/man1/watch.1

clean:
	rm -f watch watch.o core log *~
