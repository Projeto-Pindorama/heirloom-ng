all: split

split: split.o
	$(LD) $(LDFLAGS) split.o $(LCOMMON) $(LWCHAR) $(LIBS) -o split

split.o: split.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO6FL) $(IWCHAR) $(ICOMMON) $(LARGEF) -c split.c

install: all
	$(UCBINST) -c split $(ROOT)$(DEFBIN)/split
	$(STRIP) $(ROOT)$(DEFBIN)/split
	$(MANINST) -c -m 644 split.1 $(ROOT)$(MANDIR)/man1/split.1

clean:
	rm -f split split.o core log *~
