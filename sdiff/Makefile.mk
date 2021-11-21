all: sdiff

sdiff: sdiff.o
	$(LD) $(LDFLAGS) sdiff.o $(LCOMMON) $(LWCHAR) $(LIBS) -o sdiff

sdiff.o: sdiff.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO6FL) $(LARGEF) $(IWCHAR) $(ICOMMON) -c sdiff.c

install: all
	$(UCBINST) -c sdiff $(ROOT)$(DEFBIN)/sdiff
	$(STRIP) $(ROOT)$(DEFBIN)/sdiff
	$(MANINST) -c -m 644 sdiff.1 $(ROOT)$(MANDIR)/man1/sdiff.1

clean:
	rm -f sdiff sdiff.o core log *~
