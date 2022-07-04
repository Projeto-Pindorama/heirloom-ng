all: chroot

chroot: chroot.o
	$(LD) $(LDFLAGS) chroot.o $(LCOMMON) $(LIBS) -o chroot

chroot.o: chroot.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) -c chroot.c

install: all
	$(UCBINST) -c chroot $(ROOT)$(DEFSBIN)/chroot
	$(STRIP) $(ROOT)$(DEFSBIN)/chroot
	$(MANINST) -c -m 644 chroot.8 $(ROOT)$(MANDIR)/man8/chroot.8

clean:
	rm -f chroot chroot.o core log *~
