all: diff3 diff3prog

diff3: diff3.sh
	echo '#!$(SHELL)' | cat - diff3.sh | sed ' s,@DEFBIN@,$(DEFBIN),g; s,@SV3BIN@,$(SV3BIN),g; s,@DEFLIB@,$(DEFLIB),g' >diff3
	chmod 755 diff3

diff3prog: diff3prog.o
	$(LD) $(LDFLAGS) diff3prog.o $(LCOMMON) $(LIBS) -o diff3prog

diff3prog.o: diff3prog.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) -c diff3prog.c

install: all
	$(UCBINST) -c diff3 $(ROOT)$(DEFBIN)/diff3
	$(UCBINST) -c diff3prog $(ROOT)$(DEFLIB)/diff3prog
	$(STRIP) $(ROOT)$(DEFLIB)/diff3prog
	$(MANINST) -c -m 644 diff3.1 $(ROOT)$(MANDIR)/man1/diff3.1

clean:
	rm -f diff3 diff3prog diff3prog.o core log *~
