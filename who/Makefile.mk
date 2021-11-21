all: who who_sus

who: who.o
	$(LD) $(LDFLAGS) who.o $(LCOMMON) $(LWCHAR) $(LIBS) -o who

who.o: who.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(GNUFL) $(LARGEF) $(IWCHAR) $(ICOMMON) -c who.c

who_sus: who_sus.o
	$(LD) $(LDFLAGS) who_sus.o $(LCOMMON) $(LWCHAR) $(LIBS) -o who_sus

who_sus.o: who.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(GNUFL) $(LARGEF) $(IWCHAR) $(ICOMMON) -DSUS -c who.c -o who_sus.o

install: all
	$(UCBINST) -c who $(ROOT)$(SV3BIN)/who
	$(STRIP) $(ROOT)$(SV3BIN)/who
	$(UCBINST) -c who_sus $(ROOT)$(SUSBIN)/who
	$(STRIP) $(ROOT)$(SUSBIN)/who
	$(MANINST) -c -m 644 who.1 $(ROOT)$(MANDIR)/man1/who.1

clean:
	rm -f who who.o who_sus who_sus.o core log *~
