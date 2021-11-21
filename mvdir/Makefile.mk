all: mvdir

mvdir: mvdir.sh
	echo '#!$(SHELL)' | cat - mvdir.sh | sed ' s,@DEFBIN@,$(DEFBIN),g; s,@SV3BIN@,$(SV3BIN),g; s,@DEFLIB@,$(DEFLIB),g' >mvdir
	chmod 755 mvdir

install: all
	$(UCBINST) -c mvdir $(ROOT)$(DEFBIN)/mvdir
	$(MANINST) -c -m 644 mvdir.1 $(ROOT)$(MANDIR)/man1/mvdir.1

clean:
	rm -f mvdir core log *~
