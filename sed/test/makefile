DST=/usr/local/games
INSTALL=/usr/ucb/install

.SUFFIXES: .sed
.sed:
	cat $< > $@
	chmod 755 $@

all: math hanoi sokoban sedtris

install: all
	$(INSTALL) -c math $(DST)/math
	$(INSTALL) -c hanoi $(DST)/hanoi
	$(INSTALL) -c sokoban $(DST)/sokoban

clean:
	rm -f math hanoi sokoban sedtris core log *~
