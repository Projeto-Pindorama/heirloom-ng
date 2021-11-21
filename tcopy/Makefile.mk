all: tcopy

tcopy: tcopy.o
	$(LD) $(LDFLAGS) tcopy.o $(LCOMMON) $(LWCHAR) $(LIBS) -o tcopy

tcopy.o: tcopy.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(IWCHAR) $(ICOMMON) -c tcopy.c

install: all
	$(UCBINST) -c tcopy $(ROOT)$(DEFBIN)/tcopy
	$(STRIP) $(ROOT)$(DEFBIN)/tcopy
	$(MANINST) -c -m 644 tcopy.1 $(ROOT)$(MANDIR)/man1/tcopy.1

clean:
	rm -f tcopy tcopy.o core log *~
