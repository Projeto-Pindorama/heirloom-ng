all: head

head: head.o
	$(LD) $(LDFLAGS) head.o $(LCOMMON) $(LWCHAR) $(LIBS) -o head

head.o: head.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO6FL) $(LARGEF) $(IWCHAR) $(ICOMMON) -c head.c

install: all
	$(UCBINST) -c head $(ROOT)$(DEFBIN)/head
	$(STRIP) $(ROOT)$(DEFBIN)/head
	$(MANINST) -c -m 644 head.1 $(ROOT)$(MANDIR)/man1/head.1

clean:
	rm -f head head.o core log *~
