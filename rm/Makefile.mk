all: rm rm_sus

rm: rm.o
	$(LD) $(LDFLAGS) rm.o $(LCOMMON) $(LWCHAR) $(LIBS) -o rm

rm.o: rm.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(GNUFL) $(LARGEF) $(IWCHAR) $(ICOMMON) -c rm.c

rm_sus: rm_sus.o
	$(LD) $(LDFLAGS) rm_sus.o $(LCOMMON) $(LWCHAR) $(LIBS) -o rm_sus

rm_sus.o: rm.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(GNUFL) $(LARGEF) $(IWCHAR) $(ICOMMON) -DSUS -c rm.c -o rm_sus.o

install: all
	$(UCBINST) -c rm $(ROOT)$(SV3BIN)/rm
	$(STRIP) $(ROOT)$(SV3BIN)/rm
	$(UCBINST) -c rm_sus $(ROOT)$(SUSBIN)/rm
	$(STRIP) $(ROOT)$(SUSBIN)/rm
	$(MANINST) -c -m 644 rm.1 $(ROOT)$(MANDIR)/man1/rm.1

clean:
	rm -f rm rm.o rm_sus rm_sus.o core log *~
