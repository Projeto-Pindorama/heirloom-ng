all: timeout

timeout: timeout.o
	$(LD) $(LDFLAGS) timeout.o $(LCOMMON) $(LWCHAR) $(LIBS) -o timeout

timeout.o: timeout.c
	$(CC) -std=c99 -Wall $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(IWCHAR) $(ICOMMON) -I. -g -c timeout.c

install: all
	$(UCBINST) -c timeout $(ROOT)$(DEFBIN)/timeout
	$(STRIP) $(ROOT)$(DEFBIN)/timeout
	$(MANINST) -c -m 644 timeout.1 $(ROOT)$(MANDIR)/man1/timeout.1

clean:
	rm -f timeout timeout.o sigtable.o core log *~
