all: printf

printf: printf.o
	$(LD) $(LDFLAGS) printf.o $(LCOMMON) $(LWCHAR) $(LIBS) -o printf

printf.o: printf.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(GNUFL) $(LARGEF) $(IWCHAR) $(ICOMMON) -c printf.c

install: all
	$(UCBINST) -c printf $(ROOT)$(DEFBIN)/printf
	$(STRIP) $(ROOT)$(DEFBIN)/printf
	$(MANINST) -c -m 644 printf.1 $(ROOT)$(MANDIR)/man1/printf.1

clean:
	rm -f printf printf.o core log *~
