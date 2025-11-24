all: apply pick

apply.o: apply.c
	$(CC) $(CFLAGSU) $(CPPFLAGS) $(ICOMMON) -DSHELL='"$(SHELL)"' -c apply.c

apply: apply.o
	$(LD) $(LDFLAGS) apply.o $(LIBS) $(LCOMMON) -o apply

pick: pick.sh
	echo '#!$(SHELL)' | cat - pick.sh > pick
	chmod 755 pick

install: all
	$(UCBINST) -c apply $(ROOT)$(UCBBIN)/apply
	$(UCBINST) -c pick $(ROOT)$(DEFBIN)/pick
	$(UCBINST) -c -m 644 apply.1b $(ROOT)$(MANDIR)/man1b/apply.1b
	rm -f $(ROOT)$(MANDIR)/man1/pick.1
	$(LNS) $(ROOT)$(MANDIR)/man1b/apply.1b $(ROOT)$(MANDIR)/man1/pick.1

clean:
	rm -f apply apply.o pick core log *~
