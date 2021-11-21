all: time ptime

time: time.o
	$(LD) $(LDFLAGS) time.o $(LCOMMON) $(LWCHAR) $(LIBS) -o time

time.o: time.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(IWCHAR) $(ICOMMON) -c time.c

ptime: ptime.o
	$(LD) $(LDFLAGS) ptime.o $(LCOMMON) $(LWCHAR) $(LIBS) -o ptime

ptime.o: time.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(IWCHAR) $(ICOMMON) -DPTIME -c time.c -o ptime.o

install: all
	$(UCBINST) -c time $(ROOT)$(DEFBIN)/time
	$(STRIP) $(ROOT)$(DEFBIN)/time
	$(MANINST) -c -m 644 time.1 $(ROOT)$(MANDIR)/man1/time.1
	if test "`uname`" = Linux; \
	then \
		$(UCBINST) -c ptime $(ROOT)$(DEFBIN)/ptime && \
		$(STRIP) $(ROOT)$(DEFBIN)/ptime && \
		rm -f $(ROOT)$(MANDIR)/man1/ptime.1 && \
		$(LNS) time.1 $(ROOT)$(MANDIR)/man1/ptime.1 ;\
	fi

clean:
	rm -f time time.o ptime ptime.o core log *~
