all: fold

fold: fold.o
	$(LD) $(LDFLAGS) fold.o $(LCOMMON) $(LWCHAR) $(LIBS) -o fold

fold.o: fold.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO6FL) $(IWCHAR) $(ICOMMON) $(LARGEF) -c fold.c

install: all
	$(UCBINST) -c fold $(ROOT)$(DEFBIN)/fold
	$(STRIP) $(ROOT)$(DEFBIN)/fold
	$(MANINST) -c -m 644 fold.1 $(ROOT)$(MANDIR)/man1/fold.1

clean:
	rm -f fold fold.o core log *~
