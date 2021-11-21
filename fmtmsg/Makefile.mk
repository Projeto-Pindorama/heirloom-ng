all: fmtmsg

fmtmsg: fmtmsg.o main.o version.o
	$(LD) $(LDFLAGS) fmtmsg.o main.o version.o $(LCOMMON) $(LIBS) -o fmtmsg

fmtmsg.o: fmtmsg.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(ICOMMON) -c fmtmsg.c

main.o: main.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(ICOMMON) -c main.c

version.o: version.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(ICOMMON) -c version.c

install: all
	$(UCBINST) -c fmtmsg $(ROOT)$(DEFBIN)/fmtmsg
	$(STRIP) $(ROOT)$(DEFBIN)/fmtmsg
	$(MANINST) -c -m 644 fmtmsg.1 $(ROOT)$(MANDIR)/man1/fmtmsg.1

clean:
	rm -f fmtmsg fmtmsg.o main.o version.o core log *~

main.o: main.c fmtmsg.h
fmtmsg.o: fmtmsg.c fmtmsg.h
