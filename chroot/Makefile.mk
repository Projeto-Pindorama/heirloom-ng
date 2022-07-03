all: chroot

chroot: chroot.o
	$(LD) $(LDFLAGS) chroot.o $(LCOMMON) $(LIBS) -o chroot

chroot.o: chroot.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) -c chroot.c

install: all
	$(UCBINST) -c chroot $(ROOT)$(SV3BIN)/chroot
	$(STRIP) $(ROOT)$(SV3BIN)/chroot
	$(MANINST) -c -m 644 chroot.1 $(ROOT)$(MANDIR)/man1/chroot.1

clean:
	rm -f chroot chroot.o core log *~
