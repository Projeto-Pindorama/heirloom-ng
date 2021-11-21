all: dircmp

dircmp: dircmp.sh
	echo '#!$(SHELL)' | cat - dircmp.sh | sed ' s,@DEFBIN@,$(DEFBIN),g; s,@SV3BIN@,$(SV3BIN),g; s,@DEFLIB@,$(DEFLIB),g' >dircmp
	chmod 755 dircmp

install: all
	$(UCBINST) -c dircmp $(ROOT)$(DEFBIN)/dircmp
	$(MANINST) -c -m 644 dircmp.1 $(ROOT)$(MANDIR)/man1/dircmp.1

clean:
	rm -f dircmp core log *~
