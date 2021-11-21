all: users

users: users.o
	$(LD) $(LDFLAGS) users.o $(LCOMMON) $(LWCHAR) $(LIBS) -o users

users.o: users.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(GNUFL) $(LARGEF) $(IWCHAR) $(ICOMMON) -c users.c

install: all
	$(UCBINST) -c users $(ROOT)$(DEFBIN)/users
	$(STRIP) $(ROOT)$(DEFBIN)/users
	$(MANINST) -c -m 644 users.1 $(ROOT)$(MANDIR)/man1/users.1

clean:
	rm -f users users.o core log *~
