all: yes

yes: yes.o
	$(LD) $(LDFLAGS) yes.o $(LCOMMON) $(LIBS) -o yes

yes.o: yes.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) -c yes.c

install: all
	$(UCBINST) -c yes $(ROOT)$(DEFBIN)/yes
	$(STRIP) $(ROOT)$(DEFBIN)/yes
	$(MANINST) -c -m 644 yes.1 $(ROOT)$(MANDIR)/man1/yes.1

clean:
	rm -f yes yes.o core log *~
