all: timeout

timeout: timeout.o sigtable.o
	$(LD) $(LDFLAGS) timeout.o sigtable.o $(LCOMMON) $(LWCHAR) $(LIBS) -L. -o timeout

timeout.o: timeout.c
	$(CC) -std=c99 -Wall $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(IWCHAR) $(ICOMMON) -I. -c timeout.c

sigtable.o: sigtable.c
	$(CC) $(CPPFLAGS) -I. -c sigtable.c

install: all
	$(UCBINST) -c timeout $(ROOT)$(DEFBIN)/timeout
	$(STRIP) $(ROOT)$(DEFBIN)/timeout
	$(MANINST) -c -m 644 timeout.1 $(ROOT)$(MANDIR)/man1/timeout.1

clean:
	rm -f timeout timeout.o sigtable.o core log *~

sigtable.h: sigtable.c
