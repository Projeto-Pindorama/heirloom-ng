all: calendar calprog

calendar: calendar.sh
	echo '#!$(SHELL)' | cat - calendar.sh | sed ' s,@DEFBIN@,$(DEFBIN),g; s,@SV3BIN@,$(SV3BIN),g; s,@DEFLIB@,$(DEFLIB),g' >calendar
	chmod 755 calendar

calprog: calprog.o
	$(LD) $(LDFLAGS) calprog.o $(LCOMMON) $(LWCHAR) $(LIBS) -o calprog

calprog.o: calprog.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(IWCHAR) -c calprog.c

install: all
	$(UCBINST) -c calendar $(ROOT)$(DEFBIN)/calendar
	$(UCBINST) -c calprog $(ROOT)$(DEFLIB)/calprog
	$(STRIP) $(ROOT)$(DEFLIB)/calprog
	$(MANINST) -c -m 644 calendar.1 $(ROOT)$(MANDIR)/man1/calendar.1

clean:
	rm -f calendar calprog calprog.o core log *~
