all: shl

shl: shl.o
	$(LD) $(LDFLAGS) shl.o $(LCOMMON) $(LWCHAR) $(LIBS) -o shl

shl.o: shl.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(IWCHAR) $(ICOMMON) -DSV3BIN='"$(SV3BIN)"' -DDEFBIN='"$(DEFBIN)"' -c shl.c

install: all
	u=`uname`; \
	if test "$$u" != FreeBSD && test "$$u" != HP-UX && \
		test "$$u" != AIX && test "$$u" != NetBSD && \
		test "$$u" != OpenBSD && test "$$u" != DragonFly ; \
	then \
		$(UCBINST) -c $(TTYGRP) -m 2755 shl $(ROOT)$(DEFBIN)/shl &&\
		$(STRIP) $(ROOT)$(DEFBIN)/shl &&\
		$(MANINST) -c -m 644 shl.1 $(ROOT)$(MANDIR)/man1/shl.1; \
	else \
		exit 0; \
	fi

clean:
	rm -f shl shl.o core log *~
