all: units

units: units.o
	$(LD) $(LDFLAGS) units.o $(LCOMMON) $(LWCHAR) $(LIBS) -o units

units.o: units.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(IWCHAR) $(ICOMMON) -DUNITTAB='"$(DEFLIB)/unittab"' -c units.c

install: all
	$(UCBINST) -c units $(ROOT)$(DEFBIN)/units
	$(STRIP) $(ROOT)$(DEFBIN)/units
	$(MANINST) -c -m 644 units.1 $(ROOT)$(MANDIR)/man1/units.1
	$(UCBINST) -c -m 644 unittab $(ROOT)$(DEFLIB)/unittab

clean:
	rm -f units units.o core log *~
