#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License, Version 1.0 only
# (the "License").  You may not use this file except in compliance
# with the License.
#
# You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
# or http://www.opensolaris.org/os/licensing.
# See the License for the specific language governing permissions
# and limitations under the License.
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file at usr/src/OPENSOLARIS.LICENSE.
# If applicable, add the following below this CDDL HEADER, with the
# fields enclosed by brackets "[]" replaced with your own identifying
# information: Portions Copyright [yyyy] [name of copyright owner]
#
# CDDL HEADER END
#
#
# from OpenSolaris	"Makefile	1.23	05/06/08 SMI"
#
# Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
# Portions Copyright (c) 2005 Gunnar Ritter, Freiburg i. Br., Germany
#
# cmd/spell/Makefile
#

.c.o: ; $(CC) -c $(CFLAGS) $(CPPFLAGS) $(GNUFL) $(IWCHAR) $(ICOMMON) -c $<

all:	host_hashmake host_spellin spell compress spellprog spellin \
		hlista hlistb hstop hashmake hashcheck

spell: spell.sh
	echo '#!$(SHELL)' | cat - spell.sh | sed ' s,@DEFBIN@,$(DEFBIN),g; s,@SV3BIN@,$(SV3BIN),g; s,@DEFLIB@,$(DEFLIB),g; s,@SPELLHIST@,$(SPELLHIST),g' >spell
	chmod 755 spell

compress: compress.sh
	echo '#!$(SHELL)' | cat - compress.sh | sed ' s,@DEFBIN@,$(DEFBIN),g; s,@SV3BIN@,$(SV3BIN),g; s,@DEFLIB@,$(DEFLIB),g; s,@SPELLHIST@,$(SPELLHIST),g' >compress
	chmod 755 compress

spellprog: spellprog.o hash.o hashlook.o huff.o
	$(LD) $(LDFLAGS) spellprog.o hash.o hashlook.o huff.o $(LCOMMON) $(LIBS) -o spellprog

spellin: spellin.o huff.o
	$(LD) $(LDFLAGS) spellin.o huff.o $(LCOMMON) $(LIBS) -o spellin

host_spellin: spellin.c huff.c
	$(HOSTCC) spellin.c huff.c -o host_spellin

hashcheck: hashcheck.o hash.o huff.o
	$(LD) $(LDFLAGS) hashcheck.o hash.o huff.o $(LCOMMON) $(LIBS) -o hashcheck

hashmake: hashmake.o hash.o
	$(LD) $(LDFLAGS) hashmake.o hash.o $(LCOMMON) $(LIBS) -o hashmake

host_hashmake: hashmake.c hash.c
	$(HOSTCC) hashmake.c hash.c -o host_hashmake

htemp1: list local extra host_hashmake
	rm -f $@; cat list local extra | ./host_hashmake > $@

hlista: american host_spellin host_hashmake htemp1
	rm -f htemp2
	./host_hashmake <american | sort -u - htemp1 >htemp2
	rm -f $@
	./host_spellin `wc htemp2|sed -n 's/\([^ ]\) .*/\1/p'`<htemp2 >$@
	rm -f htemp2

hlistb: british host_spellin host_hashmake htemp1
	rm -f htemp2
	./host_hashmake <british | sort -u - htemp1 >htemp2
	rm -f $@
	./host_spellin `wc htemp2|sed -n 's/\([^ ]\) .*/\1/p'`<htemp2 >$@
	rm -f htemp2

hstop: stop host_spellin host_hashmake
	rm -f htemp2
	./host_hashmake <stop | sort -u >htemp2
	rm -f $@
	./host_spellin `wc htemp2|sed -n 's/\([^ ]\) .*/\1/p'`<htemp2 >$@
	rm -f htemp2

install: all
	$(UCBINST) -c spell $(ROOT)$(DEFBIN)/spell
	test -d $(ROOT)$(DEFLIB)/spell || mkdir -p $(ROOT)$(DEFLIB)/spell
	$(UCBINST) -c spellprog $(ROOT)$(DEFLIB)/spell/spellprog
	$(STRIP) $(ROOT)$(DEFLIB)/spell/spellprog
	$(UCBINST) -c spellin $(ROOT)$(DEFLIB)/spell/spellin
	$(STRIP) $(ROOT)$(DEFLIB)/spell/spellin
	rm -f $(ROOT)$(DEFLIB)/spell/spellout
	$(UCBINST) -c hashcheck $(ROOT)$(DEFLIB)/spell/hashcheck
	$(STRIP) $(ROOT)$(DEFLIB)/spell/hashcheck
	$(UCBINST) -c hashmake $(ROOT)$(DEFLIB)/spell/hashmake
	$(STRIP) $(ROOT)$(DEFLIB)/spell/hashmake
	$(UCBINST) -c compress $(ROOT)$(DEFLIB)/spell/compress
	$(UCBINST) -c -m 644 hlista $(ROOT)$(DEFLIB)/spell/hlista
	$(UCBINST) -c -m 644 hlistb $(ROOT)$(DEFLIB)/spell/hlistb
	$(UCBINST) -c -m 644 hstop $(ROOT)$(DEFLIB)/spell/hstop
	$(MANINST) -c -m 644 spell.1 $(ROOT)$(MANDIR)/man1/spell.1
	test -d `dirname $(ROOT)$(SPELLHIST)` || \
		mkdir -p `dirname $(ROOT)$(SPELLHIST)`
	-test -r $(ROOT)$(SPELLHIST) || { \
		touch $(ROOT)$(SPELLHIST); \
		chmod 666 $(ROOT)$(SPELLHIST); \
	}

clean:
	rm -f hash.o hashcheck.o hashlook.o hashmake.o huff.o \
		spellin.o spellprog.o hashcheck hashmake host_hashmake \
		host_spellin spell spellin spellprog compress \
		hlista hlistb htemp1 hstop core log *~

hash.o: hash.c hash.h
hashcheck.o: hashcheck.c hash.h huff.h
hashlook.o: hashlook.c hash.h huff.h
hashmake.o: hashmake.c hash.h
huff.o: huff.c huff.h hash.h
malloc.o: malloc.c
spellin.o: spellin.c hash.h huff.h
spellprog.o: spellprog.c hash.h
