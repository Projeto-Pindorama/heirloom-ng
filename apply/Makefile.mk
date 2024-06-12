all: apply pick

apply.o: apply.c
	$(CC) $(CFLAGS2) $(CPPFLAGS) -DSHELL='"$(SHELL)"' -std=c99 -c apply.c 
#	$(CC) -o apply_v8 -std=c99 apply.c -DSHELL='"/bin/ksh"' -g

apply: apply.o
	$(LD) $(LDFLAGS) apply.o $(LIBS) -o apply

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
	rm -f apply pick core log *~
