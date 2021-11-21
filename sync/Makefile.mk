all: sync

sync: sync.o
	$(LD) $(LDFLAGS) sync.o $(LCOMMON) $(LIBS) -o sync

sync.o: sync.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) -c sync.c

install: all
	$(UCBINST) -c sync $(ROOT)$(DEFBIN)/sync
	$(STRIP) $(ROOT)$(DEFBIN)/sync
	$(MANINST) -c -m 644 sync.1m $(ROOT)$(MANDIR)/man1m/sync.1m

clean:
	rm -f sync sync.o core log *~
