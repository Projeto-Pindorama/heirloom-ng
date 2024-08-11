all: wall

wall: wall.o
	$(LD) $(LDFLAGS) wall.o $(LCOMMON) $(LIBS) -o wall

wall.o: wall.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(ICOMMON) -c wall.c

install: all
	$(UCBINST) -c wall $(ROOT)$(DEFSBIN)/wall
	$(STRIP) $(ROOT)$(DEFSBIN)/wall
	$(MANINST) -c -m 644 wall.1m $(ROOT)$(MANDIR)/man1/wall.1m

clean:
	rm -f wall wall.o core log *~
