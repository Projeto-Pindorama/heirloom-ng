all: uniq

uniq: uniq.o
	$(LD) $(LDFLAGS) uniq.o $(LCOMMON) $(LWCHAR) $(LIBS) -o uniq

uniq.o: uniq.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO6FL) $(LARGEF) $(IWCHAR) $(ICOMMON) -c uniq.c

install: all
	$(UCBINST) -c uniq $(ROOT)$(DEFBIN)/uniq
	$(STRIP) $(ROOT)$(DEFBIN)/uniq
	$(MANINST) -c -m 644 uniq.1 $(ROOT)$(MANDIR)/man1/uniq.1

clean:
	rm -f uniq uniq.o core log *~
