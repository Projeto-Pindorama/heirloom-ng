all: man catman man.dfl

man: bang.ksh man.ksh common.ksh
	cat bang.ksh man.ksh common.ksh > man
	chmod 755 man

catman: bang.ksh man.ksh catman.ksh common.ksh
	cat bang.ksh man.ksh catman.ksh common.ksh > catman
	chmod 755 catman

bang.ksh:
	@IFS=: ;\
	if test "`uname`" = HP-UX ;\
	then \
		order='bash ksh';\
	else \
		order='ksh bash';\
	fi ;\
	for shell in ksh bash ;\
	do \
		for dir in /bin /usr/bin $$PATH ;\
		do \
			if test -x "$$dir"/$$shell ;\
			then \
				echo "selected $$dir/$$shell" ;\
				echo "#!$$dir/$$shell" > bang.ksh ;\
				echo "DFLDIR=$(DFLDIR)" >> bang.ksh ;\
				exit ;\
			fi ;\
		done ;\
	done ;\
	echo 'no bash or ksh found' >&2;\
	exit 1

man.dfl: man.dfl.in
	case `uname` in \
	Linux|*BSD) \
		mac=andoc ;; \
	*) \
		mac=an ;; \
	esac ; \
	<man.dfl.in >man.dfl sed " s,@MANDIR@,$(MANDIR),g; s,@DEFLIB@,$(DEFLIB),g; s,-mandoc,-m$$mac,"

install: all
	$(UCBINST) -c man $(ROOT)$(DEFBIN)/man
	rm -f $(ROOT)$(DEFBIN)/apropos $(ROOT)$(DEFBIN)/whatis
	$(LNS) man $(ROOT)$(DEFBIN)/apropos
	$(LNS) man $(ROOT)$(DEFBIN)/whatis
	$(UCBINST) -c catman $(ROOT)$(DEFSBIN)/catman
	test -d $(ROOT)$(DEFLIB)/tmac || mkdir -p $(ROOT)$(DEFLIB)/tmac
	$(UCBINST) -c -m 644 tmac.an $(ROOT)$(DEFLIB)/tmac/tmac.an
	rm -f $(ROOT)$(DEFLIB)/tmac/an-old.tmac
	$(LNS) tmac.an $(ROOT)$(DEFLIB)/tmac/an-old.tmac
	test -r $(ROOT)$(DFLDIR)/man || $(UCBINST) -c -m 644 man.dfl $(ROOT)$(DFLDIR)/man
	$(MANINST) -c -m 644 man.1 $(ROOT)$(MANDIR)/man1/man.1
	$(MANINST) -c -m 644 apropos.1 $(ROOT)$(MANDIR)/man1/apropos.1
	$(MANINST) -c -m 644 whatis.1 $(ROOT)$(MANDIR)/man1/whatis.1
	$(MANINST) -c -m 644 man.7 $(ROOT)$(MANDIR)/man7/man.7
	$(MANINST) -c -m 644 catman.8 $(ROOT)$(MANDIR)/man8/catman.8

clean:
	rm -f man catman bang.ksh man.dfl core log *~
