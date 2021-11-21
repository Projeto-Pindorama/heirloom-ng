all: pgrep pkill

pgrep: pgrep.o
	$(LD) $(LDFLAGS) pgrep.o $(LUXRE) $(LCOMMON) $(LWCHAR) $(LKVM) $(LIBS) -o pgrep

pgrep.o: pgrep.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO6FL) $(IUXRE) $(IWCHAR) $(ICOMMON) -c pgrep.c

pkill: pgrep
	rm -f pkill
	$(LNS) pgrep pkill

install: all
	$(UCBINST) -c pgrep $(ROOT)$(DEFBIN)/pgrep
	$(STRIP) $(ROOT)$(DEFBIN)/pgrep
	rm -f $(ROOT)$(DEFBIN)/pkill
	$(LNS) pgrep $(ROOT)$(DEFBIN)/pkill
	$(MANINST) -c -m 644 pgrep.1 $(ROOT)$(MANDIR)/man1/pgrep.1
	rm -f $(ROOT)$(MANDIR)/man1/pkill.1
	$(LNS) pgrep.1 $(ROOT)$(MANDIR)/man1/pkill.1

clean:
	rm -f pgrep pgrep.o pkill core log *~
