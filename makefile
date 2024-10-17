SHELL = /bin/sh

SUBDIRS = build libwchar libcommon libuxre _install \
	apply banner basename bc bdiff bfs \
	cal calendar cat chmod chown chroot \
	cksum cmp col comm copy cp cpio csplit cut \
	date dc dd deroff diff diff3 dircmp dirname df du \
	echo ed env expand expr \
	factor file find fmt fmtmsg fold \
	getconf getopt grep groups hd head hostname id join \
	kill line listusers ln logins logname ls \
	mail man mesg mkdir mkfifo mknod more mvdir \
	nawk news nice nl nohup oawk od \
	paste pathchk pg pgrep pr printenv printf priocntl ps psrinfo pwd \
	random readlink renice rev rm rmdir \
	sdiff sed seq setpgrp shl sleep sort spell split stty su sum sync \
	tabs tail tapecntl tar tcopy tee test time timeout touch tr true tsort tty \
	ul uname uniq units users watch wc what who whoami whodo xargs yes

dummy: makefiles all

.DEFAULT:
	+ for i in $(SUBDIRS) ;\
	do	\
		(cd "$$i" && $(MAKE) $@) || exit ; \
	done
	$(MAKE) -f Makefile $@

.SUFFIXES: .mk
.mk:
	cat build/mk.head build/mk.config $< build/mk.tail >$@

makefiles: Makefile $(SUBDIRS:=/Makefile)

install:
	$(MAKE) -f Makefile directories
	+ for i in $(SUBDIRS) ;\
	do	\
		(cd "$$i" && $(MAKE) $@) || exit ; \
	done
	$(MAKE) -f Makefile links

mrproper:
	rm -f .foo .FOO Makefile
	+ for i in $(SUBDIRS) ;\
	do	\
		(cd "$$i" && $(MAKE) $@) || exit ; \
	done

casecheck: .foo .Foo
	cmp -s .foo .Foo && exit 1 || exit 0

.foo .Foo:
	echo $@ > $@

PKGROOT =	/var/tmp/heirloom-root
PKGTEMP =	/var/tmp
PKGPROTO =	prototype

ou8:
	$(MAKE) pkgbuild LEX=flex

heirloom.pkg: all
	rm -rf $(PKGROOT)
	mkdir $(PKGROOT)
	$(MAKE) $(PKGFLAGS) ROOT=$(PKGROOT) install
	mkdir -p $(PKGROOT)/usr/share/doc/heirloom
	(echo README; find * -name NOTES -print; \
			find LICENSE/* -print) | \
		cpio -pdm $(PKGROOT)/usr/share/doc/heirloom
	rm -f $(PKGPROTO)
	echo 'i pkginfo' >$(PKGPROTO)
	(cd $(PKGROOT) && find . -print | pkgproto) | >>$(PKGPROTO) sed 's:^\([df] [^ ]* [^ ]* [^ ]*\) .*:\1 root root:; s:^\(f [^ ]* [^ ]*/ps \).*:\14755 root root:; s:^\(f [^ ]* [^ ]*/shl \).*:\12755 root adm:; s:^\(f [^ ]* [^ ]*/su \).*:\14755 root root:; s:^f\( [^ ]* etc/\):v \1:; s:^f\( [^ ]* var/\):v \1:; s:^\(s [^ ]* [^ ]*=\)\([^/]\):\1./\2:'
	rm -rf $(PKGTEMP)/$@
	pkgmk -a `uname -m` -d $(PKGTEMP) -r $(PKGROOT) -f $(PKGPROTO) $@
	pkgtrans -o -s $(PKGTEMP) `pwd`/$@ $@
	rm -rf $(PKGROOT)
	rm -rf $(PKGPROTO) $(PKGTEMP)/heirloom

DIETFLAGS =	CC="$(HOME)/src/diet gcc" HOSTCC="$(HOME)/src/diet gcc" \
		CFLAGS="-Os -fomit-frame-pointer" \
		CFLAGSS="-Os -fomit-frame-pointer" \
		CFLAGS2="-Os -fomit-frame-pointer" \
		CFLAGSU="-Os -fomit-frame-pointer" \
		LCRYPT= \
		IWCHAR=-I../libwchar LWCHAR="-L../libwchar -lwchar" \
		DEFBIN=/5bin SV3BIN=/5bin S42BIN=/5bin/s42 \
		SUSBIN=/5bin/posix SU3BIN=/5bin/posix2001 UCBBIN=/5bin/ucb \
		CCSBIN=/5bin/ccs \
		DEFLIB=/5bin/lib DEFSBIN=/5bin MANDIR=/tmp/__man__ \
		DFLDIR=/etc/default SPELLHIST=/var/adm/spellhist \
		SULOG=/var/log/sulog MAGIC=/5bin/lib/magic

diet:
	$(MAKE) $(DIETFLAGS)

dietinstall:
	$(MAKE) $(DIETFLAGS) install
	rm -rf /tmp/__man__

world:
	make mrproper
	make diet
	sudo make dietinstall
	make mrproper
	make
	sudo make install
	make clean

freebsd:
	$(MAKE) LKVM=-lkvm \
	XO5FL= XO6FL= GNUFL= YACC=yacc

netbsd:
	$(MAKE) LKVM=-lkvm WERROR= \
	XO5FL= XO6FL= GNUFL= YACC=yacc LCURS=-ltermcap CPPFLAGS=-DUSE_TERMCAP

pie:
	$(MAKE) \
		CFLAGS="-Os -fomit-frame-pointer -fPIE" \
		CFLAGSS="-Os -fomit-frame-pointer -fPIE" \
		CFLAGS2="-Os -fomit-frame-pointer -fPIE" \
		LDFLAGS="-pie"

ps2:
	$(MAKE) WARN="-Wchar-subscripts -Wimplicit \
			-Wmissing-braces -Wreturn-type -Wtrigraphs \
			-Wuninitialized -Wmultichar -Wpointer-arith -Werror"

ps2diet:
	$(MAKE) CC="$(HOME)/src/diet gcc" \
		CFLAGS="-Os -fomit-frame-pointer" \
		CFLAGSS="-Os -fomit-frame-pointer" \
		CFLAGS2="-Os -fomit-frame-pointer" \
		CFLAGSU="-Os -fomit-frame-pointer" \
		LCRYPT= \
		IWCHAR=-I../libwchar LWCHAR="-L../libwchar -lwchar" \
		WARN="-Wchar-subscripts -Wimplicit \
			-Wmissing-braces -Wreturn-type -Wtrigraphs \
			-Wuninitialized -Wmultichar -Wpointer-arith -Werror"
