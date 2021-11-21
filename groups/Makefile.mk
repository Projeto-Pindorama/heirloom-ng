all: groups groups_ucb

groups: groups.o
	$(LD) $(LDFLAGS) groups.o $(LCOMMON) $(LWCHAR) $(LIBS) -o groups

groups.o: groups.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(GNUFL) $(IWCHAR) $(ICOMMON) -c groups.c

groups_ucb: groups_ucb.o
	$(LD) $(LDFLAGS) groups_ucb.o $(LCOMMON) $(LWCHAR) $(LIBS) -o groups_ucb

groups_ucb.o: groups.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(GNUFL) $(IWCHAR) $(ICOMMON) -DUCB -c groups.c -o groups_ucb.o

install: all
	$(UCBINST) -c groups $(ROOT)$(DEFBIN)/groups
	$(STRIP) $(ROOT)$(DEFBIN)/groups
	$(UCBINST) -c groups_ucb $(ROOT)$(UCBBIN)/groups
	$(STRIP) $(ROOT)$(UCBBIN)/groups
	$(MANINST) -c -m 644 groups.1 $(ROOT)$(MANDIR)/man1/groups.1
	$(MANINST) -c -m 644 groups.1b $(ROOT)$(MANDIR)/man1b/groups.1b

clean:
	rm -f groups groups.o groups_ucb groups_ucb.o core log *~
