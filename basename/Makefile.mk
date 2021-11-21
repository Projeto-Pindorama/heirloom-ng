all: basename basename_sus basename_ucb

basename: basename.o
	$(LD) $(LDFLAGS) basename.o $(LCOMMON) $(LWCHAR) $(LIBS) -o basename

basename.o: basename.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(IWCHAR) $(ICOMMON) -c basename.c

basename_sus: basename_sus.o
	$(LD) $(LDFLAGS) basename_sus.o $(LCOMMON) $(LWCHAR) $(LIBS) -o basename_sus

basename_sus.o: basename.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(IWCHAR) -DSUS -c basename.c -o basename_sus.o

basename_ucb: basename_ucb.o
	$(LD) $(LDFLAGS) basename_ucb.o $(LCOMMON) $(LWCHAR) $(LIBS) -o basename_ucb

basename_ucb.o: basename.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(IWCHAR) -DUCB -c basename.c -o basename_ucb.o

install: all
	$(UCBINST) -c basename $(ROOT)$(SV3BIN)/basename
	$(STRIP) $(ROOT)$(SV3BIN)/basename
	$(UCBINST) -c basename_sus $(ROOT)$(SUSBIN)/basename
	$(STRIP) $(ROOT)$(SUSBIN)/basename
	$(UCBINST) -c basename_ucb $(ROOT)$(UCBBIN)/basename
	$(STRIP) $(ROOT)$(UCBBIN)/basename
	$(MANINST) -c -m 644 basename.1 $(ROOT)$(MANDIR)/man1/basename.1
	$(MANINST) -c -m 644 basename.1b $(ROOT)$(MANDIR)/man1b/basename.1b

clean:
	rm -f basename basename.o basename_sus basename_sus.o basename_ucb basename_ucb.o core log *~
