all: cmp

cmp: cmp.o
	$(LD) $(LDFLAGS) cmp.o $(LCOMMON) $(LWCHAR) $(LIBS) -o cmp

cmp.o: cmp.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO6FL) $(LARGEF) $(IWCHAR) $(ICOMMON) -c cmp.c

install: all
	$(UCBINST) -c cmp $(ROOT)$(DEFBIN)/cmp
	$(STRIP) $(ROOT)$(DEFBIN)/cmp
	$(MANINST) -c -m 644 cmp.1 $(ROOT)$(MANDIR)/man1/cmp.1

clean:
	rm -f cmp cmp.o core log *~
