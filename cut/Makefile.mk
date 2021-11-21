all: cut

cut: cut.o
	$(LD) $(LDFLAGS) cut.o $(LCOMMON) $(LWCHAR) $(LIBS) -o cut

cut.o: cut.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO6FL) $(IWCHAR) $(ICOMMON) $(LARGEF) -c cut.c

install: all
	$(UCBINST) -c cut $(ROOT)$(DEFBIN)/cut
	$(STRIP) $(ROOT)$(DEFBIN)/cut
	$(MANINST) -c -m 644 cut.1 $(ROOT)$(MANDIR)/man1/cut.1

clean:
	rm -f cut cut.o core log *~
