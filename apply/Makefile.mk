all: apply pick

apply:
	@echo "apply(1b) is not implemented, yet"

pick: pick.sh
	echo '#!$(SHELL)' | cat - pick.sh > pick
	chmod 755 pick

install: all
	# $(UCBINST) -c apply $(ROOT)$(DEFBIN)/apply
	$(UCBINST) -c pick $(ROOT)$(DEFBIN)/pick
	# $(UCBINST) -c -m 644 apply.1b $(ROOT)$(MANDIR)/man1b/apply.1b
	# rm -f $(ROOT)$(MANDIR)/man1/pick.1
	# $(LNS) $(ROOT)$(MANDIR)/man1b/apply.1b $(ROOT)$(MANDIR)/man1/pick.1

clean:
	rm -f apply pick core log *~
