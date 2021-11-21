all: date date_sus

date: date.o
	$(LD) $(LDFLAGS) date.o $(LCOMMON) $(LWCHAR) $(LIBS) -o date

date.o: date.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(GNUFL) $(IWCHAR) $(ICOMMON) -c date.c

date_sus: date_sus.o
	$(LD) $(LDFLAGS) date_sus.o $(LCOMMON) $(LWCHAR) $(LIBS) -o date_sus

date_sus.o: date.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(GNUFL) $(IWCHAR) $(ICOMMON) -DSUS -c date.c -o date_sus.o

install: all
	$(UCBINST) -c date $(ROOT)$(SV3BIN)/date
	$(STRIP) $(ROOT)$(SV3BIN)/date
	$(UCBINST) -c date_sus $(ROOT)$(SUSBIN)/date
	$(STRIP) $(ROOT)$(SUSBIN)/date
	$(MANINST) -c -m 644 date.1 $(ROOT)$(MANDIR)/man1/date.1

clean:
	rm -f date date.o date_sus date_sus.o core log *~
