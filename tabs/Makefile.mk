all: tabs newform

tabs: tabs.o tabspec.o
	$(LD) $(LDFLAGS) tabs.o tabspec.o $(LCURS) $(LCOMMON) $(LWCHAR) $(LIBS) -o tabs

tabs.o: tabs.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO6FL) $(LARGEF) $(IWCHAR) $(ICOMMON) -c tabs.c

newform: newform.o tabspec.o
	$(LD) $(LDFLAGS) newform.o tabspec.o $(LCOMMON) $(LWCHAR) $(LIBS) -o newform

newform.o: newform.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO6FL) $(LARGEF) $(IWCHAR) $(ICOMMON) -c newform.c

tabspec.o: tabspec.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO6FL) $(LARGEF) $(IWCHAR) $(ICOMMON) -c tabspec.c

install: all
	$(UCBINST) -c tabs $(ROOT)$(DEFBIN)/tabs
	$(STRIP) $(ROOT)$(DEFBIN)/tabs
	$(UCBINST) -c newform $(ROOT)$(DEFBIN)/newform
	$(STRIP) $(ROOT)$(DEFBIN)/newform
	$(MANINST) -c -m 644 tabs.1 $(ROOT)$(MANDIR)/man1/tabs.1
	$(MANINST) -c -m 644 newform.1 $(ROOT)$(MANDIR)/man1/newform.1
	$(MANINST) -c -m 644 fspec.5 $(ROOT)$(MANDIR)/man5/fspec.5

clean:
	rm -f tabs tabs.o newform newform.o tabspec.o core log *~

tabs.o: tabspec.h
newform.o: tabspec.h
tabspec.o: tabspec.h
