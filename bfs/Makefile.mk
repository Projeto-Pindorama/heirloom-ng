.c.o: ; $(CC) -c $(CFLAGS) $(CPPFLAGS) $(GNUFL) $(LARGEF) $(ICOMMON) $(IWCHAR) -DSHELL='"$(SHELL)"' $<

all: bfs

bfs: bfs.o
	$(CC) $(LDFLAGS) bfs.o $(LCOMMON) $(LWCHAR) $(LIBS) -o bfs

install: all
	test -d $(ROOT)$(SV3BIN) || mkdir -p $(ROOT)$(SV3BIN)
	$(UCBINST) -c -m 755 bfs $(ROOT)$(SV3BIN)/bfs
	$(STRIP) $(ROOT)$(SV3BIN)/bfs
	$(MANINST) -c -m 644 bfs.1 $(ROOT)$(MANDIR)/man1/bfs.1

clean:
	rm -f bfs bfs.o core log *~
