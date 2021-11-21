all: dd

dd: dd.o
	$(LD) $(LDFLAGS) dd.o $(LCOMMON) $(LWCHAR) $(LIBS) -o dd

dd.o: dd.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(GNUFL) $(LARGEF) $(IWCHAR) $(ICOMMON) -c dd.c

install: all
	$(UCBINST) -c dd $(ROOT)$(DEFBIN)/dd
	$(STRIP) $(ROOT)$(DEFBIN)/dd
	$(MANINST) -c -m 644 dd.1 $(ROOT)$(MANDIR)/man1/dd.1

clean:
	rm -f dd dd.o core log *~
