all: tsort

tsort: tsort.o
	$(LD) $(LDFLAGS) tsort.o $(LCOMMON) $(LWCHAR) $(LIBS) -o tsort

tsort.o: tsort.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(IWCHAR) $(ICOMMON) -c tsort.c

install: all
	$(UCBINST) -c tsort $(ROOT)$(CCSBIN)/tsort
	$(STRIP) $(ROOT)$(CCSBIN)/tsort
	$(MANINST) -c -m 644 tsort.1 $(ROOT)$(MANDIR)/man1/tsort.1

clean:
	rm -f tsort tsort.o core log *~
