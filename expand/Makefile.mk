all: expand unexpand

expand: expand.o tablist.o
	$(LD) $(LDFLAGS) expand.o tablist.o $(LCOMMON) $(LWCHAR) $(LIBS) -o expand

expand.o: expand.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO6FL) $(IWCHAR) $(ICOMMON) $(LARGEF) -c expand.c

unexpand: unexpand.o tablist.o
	$(LD) $(LDFLAGS) unexpand.o tablist.o $(LCOMMON) $(LWCHAR) $(LIBS) -o unexpand

unexpand.o: unexpand.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO6FL) $(IWCHAR) $(ICOMMON) $(LARGEF) -c unexpand.c

tablist.o: tablist.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO6FL) $(IWCHAR) $(ICOMMON) $(LARGEF) -c tablist.c

install: all
	$(UCBINST) -c expand $(ROOT)$(DEFBIN)/expand
	$(STRIP) $(ROOT)$(DEFBIN)/expand
	$(UCBINST) -c unexpand $(ROOT)$(DEFBIN)/unexpand
	$(STRIP) $(ROOT)$(DEFBIN)/unexpand
	$(MANINST) -c -m 644 expand.1 $(ROOT)$(MANDIR)/man1/expand.1
	$(MANINST) -c -m 644 unexpand.1 $(ROOT)$(MANDIR)/man1/unexpand.1

clean:
	rm -f expand expand.o unexpand unexpand.o tablist.o core log *~

expand.c: tablist.h
unexpand.c: tablist.h
tablist.o: tablist.h
