all: maninst crossln genintro

maninst: maninst.sh mk.config
	<maninst.sh >maninst sed ' s,@DEFLIB@,$(DEFLIB),g; s,@SPELLHIST@,$(SPELLHIST),g; s,@MAGIC@,$(MAGIC),g; s,@DFLDIR@,$(DFLDIR),g; s,@DEFBIN@,$(DEFBIN),g; s,@SV3BIN@,$(SV3BIN),g; s,@S42BIN@,$(S42BIN),g; s,@SUSBIN@,$(SUSBIN),g; s,@SU3BIN@,$(SU3BIN),g; s,@UCBBIN@,$(UCBBIN),g; s,@CCSBIN@,$(CCSBIN),g'
	chmod 755 maninst

crossln: crossln.sh
	<crossln.sh >crossln sed 's,@LNS@,$(LNS),g'
	chmod 755 crossln

genintro: genintro.sh
	cat genintro.sh >genintro
	chmod 755 genintro

install:

clean:
	rm -f core log *~

MRPROPER = maninst crossln genintro
