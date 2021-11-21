all: col

col: col.o
	$(LD) $(LDFLAGS) col.o $(LCOMMON) $(LWCHAR) $(LIBS) -o col

col.o: col.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO6FL) $(IWCHAR) $(ICOMMON) $(LARGEF) -c col.c

install: all
	$(UCBINST) -c col $(ROOT)$(DEFBIN)/col
	$(STRIP) $(ROOT)$(DEFBIN)/col
	$(MANINST) -c -m 644 col.1 $(ROOT)$(MANDIR)/man1/col.1

clean:
	rm -f col col.o core log *~
