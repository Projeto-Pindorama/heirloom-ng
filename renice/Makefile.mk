all: renice

renice: renice.o
	$(LD) $(LDFLAGS) renice.o $(LCOMMON) $(LIBS) -o renice

renice.o: renice.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) -c renice.c

install: all
	$(UCBINST) -c renice $(ROOT)$(DEFBIN)/renice
	$(STRIP) $(ROOT)$(DEFBIN)/renice
	$(MANINST) -c -m 644 renice.1 $(ROOT)$(MANDIR)/man1/renice.1

clean:
	rm -f renice renice.o core log *~
