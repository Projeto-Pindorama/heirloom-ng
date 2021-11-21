all: kill

kill: kill.o strsig.o version.o
	$(LD) $(LDFLAGS) kill.o strsig.o version.o $(LCOMMON) $(LIBS) -o kill

kill.o: kill.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(ICOMMON) -c kill.c

strsig.o: strsig.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(ICOMMON) -c strsig.c

version.o: version.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(ICOMMON) -c version.c

install: all
	$(UCBINST) -c kill $(ROOT)$(DEFBIN)/kill
	$(STRIP) $(ROOT)$(DEFBIN)/kill
	$(MANINST) -c -m 644 kill.1 $(ROOT)$(MANDIR)/man1/kill.1

clean:
	rm -f kill kill.o strsig.o version.o core log *~
