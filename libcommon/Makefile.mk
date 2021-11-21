all: libcommon.a

OBJ = asciitype.o ib_alloc.o ib_close.o ib_free.o ib_getlin.o ib_getw.o \
	ib_open.o ib_popen.o ib_read.o ib_seek.o oblok.o sfile.o strtol.o \
	getdir.o regexpr.o gmatch.o utmpx.o memalign.o pathconf.o \
	sigset.o signal.o sigrelse.o sighold.o sigignore.o sigpause.o \
	getopt.o pfmt.o vpfmt.o setlabel.o setuxlabel.o pfmt_label.o sysv3.o
libcommon.a: headers $(OBJ)
	$(AR) -rv $@ $(OBJ)
	$(RANLIB) $@

CHECK: CHECK.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LARGEF) -E CHECK.c >CHECK

headers: CHECK
	one() { \
		rm -f "$$1.h"; \
		if grep "$$1_h[	 ]*=[ 	]*[^0][	 ]*;" CHECK >/dev/null; \
		then \
			ln -s "_$$1.h" "$$1.h"; \
		fi; \
	}; \
	one alloca; \
	one malloc; \
	one utmpx

install:

clean:
	rm -f $(OBJ) core log *~ alloca.h malloc.h utmpx.h CHECK

asciitype.o: asciitype.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(IWCHAR) -I. -c asciitype.c

getdir.o: getdir.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(IWCHAR) -I. -c getdir.c

ib_alloc.o: ib_alloc.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(IWCHAR) -I. -c ib_alloc.c

ib_close.o: ib_close.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(IWCHAR) -I. -c ib_close.c

ib_free.o: ib_free.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(IWCHAR) -I. -c ib_free.c

ib_getlin.o: ib_getlin.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(IWCHAR) -I. -c ib_getlin.c

ib_getw.o: ib_getw.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(IWCHAR) -I. -c ib_getw.c

ib_open.o: ib_open.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(IWCHAR) -I. -c ib_open.c

ib_popen.o: ib_popen.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(IWCHAR) -I. -c ib_popen.c

ib_read.o: ib_read.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(IWCHAR) -I. -c ib_read.c

ib_seek.o: ib_seek.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(IWCHAR) -I. -c ib_seek.c

oblok.o: oblok.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(IWCHAR) -I. -c oblok.c

regexpr.o: regexpr.c
	$(CC) $(CFLAGS2) $(CPPFLAGS) $(LARGEF) $(IWCHAR) -I. -c regexpr.c

sigset.o: sigset.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(IWCHAR) -I. -c sigset.c

signal.o: signal.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(IWCHAR) -I. -c signal.c

sigignore.o: sigignore.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(IWCHAR) -I. -c sigignore.c

sigpause.o: sigpause.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(IWCHAR) -I. -c sigpause.c

sigrelse.o: sigrelse.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(IWCHAR) -I. -c sigrelse.c

sighold.o: sighold.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(IWCHAR) -I. -c sighold.c

gmatch.o: gmatch.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(IWCHAR) -I. -c gmatch.c

utmpx.o: utmpx.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(IWCHAR) -I. -c utmpx.c

memalign.o: memalign.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(IWCHAR) -I. -c memalign.c

pathconf.o: pathconf.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(IWCHAR) -I. -c pathconf.c

strtol.o: strtol.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(IWCHAR) -I. -c strtol.c

getopt.o: getopt.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(IWCHAR) -I. -c getopt.c

sysv3.o: sysv3.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(IWCHAR) -I. -c sysv3.c

sfile.o: sfile.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(IWCHAR) -I. -c sfile.c

pfmt.o: pfmt.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(IWCHAR) -I. -c pfmt.c

vpfmt.o: vpfmt.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(IWCHAR) -I. -c vpfmt.c

setlabel.o: setlabel.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(IWCHAR) -I. -c setlabel.c

setuxlabel.o: setuxlabel.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(IWCHAR) -I. -c setuxlabel.c

pfmt_label.o: pfmt_label.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(IWCHAR) -I. -c pfmt_label.c

asciitype.o: asciitype.h
ib_alloc.o: iblok.h
ib_close.o: iblok.h
ib_free.o: iblok.h
ib_getlin.o: iblok.h
ib_getw.o: iblok.h
ib_open.o: iblok.h
ib_read.o: iblok.h
ib_seek.o: iblok.h
iblok.o: iblok.h
oblok.o: oblok.h
sfile.o: sfile.h
getdir.o: getdir.h
regexpr.o: regexpr.h regexp.h
pfmt.o: pfmt.h
vpfmt.o: pfmt.h
setlabel.o: pfmt.h
setuxlabel.o: pfmt.h msgselect.h
getopt.o: msgselect.h
sighold.o: sigset.h
sigignore.o: sigset.h
sigpause.o: sigset.h
sigrelse.o: sigset.h
sigset.o: sigset.h
signal.o: sigset.h
pathconf.o: pathconf.h

MRPROPER = libcommon.a
