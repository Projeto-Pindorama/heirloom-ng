all: cal

cal: cal.o
	$(LD) $(LDFLAGS) cal.o $(LCOMMON) $(LWCHAR) $(LIBS) -o cal

cal.o: cal.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(IWCHAR) -c cal.c

install: all
	$(UCBINST) -c cal $(ROOT)$(DEFBIN)/cal
	$(STRIP) $(ROOT)$(DEFBIN)/cal
	$(MANINST) -c -m 644 cal.1 $(ROOT)$(MANDIR)/man1/cal.1

clean:
	rm -f cal cal.o core log *~
