all: true false

true: true.sh
	echo '#!$(SHELL)' | cat - true.sh >true
	chmod 755 true
false: false.sh
	echo '#!$(SHELL)' | cat - false.sh >false
	chmod 755 false

install: all
	$(UCBINST) -c true $(ROOT)$(DEFBIN)/true
	$(UCBINST) -c false $(ROOT)$(DEFBIN)/false
	$(MANINST) -c -m 644 true.1 $(ROOT)$(MANDIR)/man1/true.1
	rm -f $(ROOT)$(MANDIR)/man1/false.1
	$(LNS) true.1 $(ROOT)$(MANDIR)/man1/false.1

clean:
	rm -f true false core log *~
