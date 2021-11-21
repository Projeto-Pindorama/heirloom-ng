all: hd

hd: hd.o
	$(LD) $(LDFLAGS) hd.o $(LCOMMON) $(LWCHAR) $(LIBS) -o hd

hd.o: hd.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO6FL) $(LARGEF) $(IWCHAR) $(ICOMMON) -c hd.c

install: all
	$(UCBINST) -c hd $(ROOT)$(DEFBIN)/hd
	$(STRIP) $(ROOT)$(DEFBIN)/hd
	$(MANINST) -c -m 644 hd.1 $(ROOT)$(MANDIR)/man1/hd.1

clean:
	rm -f hd hd.o core log *~
