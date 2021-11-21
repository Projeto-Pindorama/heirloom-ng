all: comm

comm: comm.o
	$(LD) $(LDFLAGS) comm.o $(LCOMMON) $(LWCHAR) $(LIBS) -o comm

comm.o: comm.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO6FL) $(LARGEF) $(IWCHAR) $(ICOMMON) -c comm.c

install: all
	$(UCBINST) -c comm $(ROOT)$(DEFBIN)/comm
	$(STRIP) $(ROOT)$(DEFBIN)/comm
	$(MANINST) -c -m 644 comm.1 $(ROOT)$(MANDIR)/man1/comm.1

clean:
	rm -f comm comm.o core log *~
