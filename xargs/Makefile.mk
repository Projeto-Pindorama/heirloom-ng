all: xargs

xargs: xargs.o
	$(LD) $(LDFLAGS) xargs.o $(LCOMMON) $(LWCHAR) $(LIBS) -o xargs

xargs.o: xargs.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO6FL) $(LARGEF) $(IWCHAR) $(ICOMMON) -c xargs.c

install: all
	$(UCBINST) -c xargs $(ROOT)$(DEFBIN)/xargs
	$(STRIP) $(ROOT)$(DEFBIN)/xargs
	$(MANINST) -c -m 644 xargs.1 $(ROOT)$(MANDIR)/man1/xargs.1

clean:
	rm -f xargs xargs.o core log *~
