all: more

more: more.o
	$(LD) $(LDFLAGS) more.o $(LCURS) $(LCOMMON) $(LUXRE) $(LWCHAR) $(LIBS) -o more

more.o: more.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO6FL) $(LARGEF) $(IWCHAR) $(ICOMMON) $(IUXRE) -c more.c

install: all
	$(UCBINST) -c more $(ROOT)$(SV3BIN)/more
	$(STRIP) $(ROOT)$(SV3BIN)/more
	rm -f $(ROOT)$(SV3BIN)/page
	$(LNS) more $(ROOT)$(SV3BIN)/page
	$(MANINST) -c -m 644 more.1 $(ROOT)$(MANDIR)/man1/more.1
	rm -f $(ROOT)$(MANDIR)/man1/page.1
	$(LNS) more.1 $(ROOT)$(MANDIR)/man1/page.1

clean:
	rm -f more more.o core log *~
