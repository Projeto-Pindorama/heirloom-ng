all: mknod

mknod: mknod.o
	$(LD) $(LDFLAGS) mknod.o $(LCOMMON) $(LIBS) -o mknod

mknod.o: mknod.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(GNUFL) $(LARGEF) -c mknod.c

install: all
	$(UCBINST) -c mknod $(ROOT)$(DEFBIN)/mknod
	$(STRIP) $(ROOT)$(DEFBIN)/mknod
	$(MANINST) -c -m 644 mknod.1m $(ROOT)$(MANDIR)/man1m/mknod.1m

clean:
	rm -f mknod mknod.o core log *~
