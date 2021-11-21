all: banner

banner: banner.o
	$(LD) $(LDFLAGS) banner.o $(LCOMMON) $(LIBS) -o banner

banner.o: banner.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) -c banner.c

install: all
	$(UCBINST) -c banner $(ROOT)$(DEFBIN)/banner
	$(STRIP) $(ROOT)$(DEFBIN)/banner
	$(MANINST) -c -m 644 banner.1 $(ROOT)$(MANDIR)/man1/banner.1

clean:
	rm -f banner banner.o core log *~
