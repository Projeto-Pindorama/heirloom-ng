all: psrinfo

psrinfo: psrinfo.o
	$(LD) $(LDFLAGS) psrinfo.o $(LCOMMON) $(LIBS) -o psrinfo

psrinfo.o: psrinfo.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO6FL) -c psrinfo.c

install: all
	if test "`uname`" = Linux; \
	then \
		$(UCBINST) -c psrinfo $(ROOT)$(DEFBIN)/psrinfo && \
		$(STRIP) $(ROOT)$(DEFBIN)/psrinfo && \
		$(MANINST) -c -m 644 psrinfo.1 $(ROOT)$(MANDIR)/man1/psrinfo.1;\
	else \
		exit 0; \
	fi

clean:
	rm -f psrinfo psrinfo.o core log *~
