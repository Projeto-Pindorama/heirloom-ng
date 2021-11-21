all: sleep

sleep: sleep.o
	$(LD) $(LDFLAGS) sleep.o $(LCOMMON) $(LIBS) -o sleep

sleep.o: sleep.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) -c sleep.c

install: all
	$(UCBINST) -c sleep $(ROOT)$(DEFBIN)/sleep
	$(STRIP) $(ROOT)$(DEFBIN)/sleep
	$(MANINST) -c -m 644 sleep.1 $(ROOT)$(MANDIR)/man1/sleep.1

clean:
	rm -f sleep sleep.o core log *~
