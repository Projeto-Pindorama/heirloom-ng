all: awk awk_sus awk_su3

OBJ = awk.lx.o b.o lib.o main.o parse.o proctab.o run.o tran.o

awk: awk.g.o $(OBJ) version.o
	$(LD) $(LDFLAGS) awk.g.o $(OBJ) version.o $(LUXRE) -lm $(LCOMMON) $(LWCHAR) $(LIBS) -o awk

awk_sus: awk.g.o $(OBJ) version_sus.o
	$(LD) $(LDFLAGS) awk.g.o $(OBJ) version_sus.o $(LUXRE) -lm $(LCOMMON) $(LWCHAR) $(LIBS) -o awk_sus

awk_su3: awk.g.2001.o $(OBJ) version_su3.o
	$(LD) $(LDFLAGS) awk.g.2001.o $(OBJ) version_su3.o $(LUXRE) -lm $(LCOMMON) $(LWCHAR) $(LIBS) -o awk_su3

awk.g.c: awk.g.y
	$(YACC) -d awk.g.y
	mv -f y.tab.c awk.g.c
	(echo '1i'; echo '#include <inttypes.h>'; echo '.'; echo 'w';) | \
		ed -s y.tab.h

awk.g.2001.c: awk.g.2001.y awk.g.c
	$(YACC) awk.g.2001.y
	mv -f y.tab.c awk.g.2001.c

awk.g.2001.y: awk.g.y
	sed -f rerule.sed <awk.g.y >awk.g.2001.y

maketab: maketab.o
	$(HOSTCC) maketab.o -o maketab
	./maketab > proctab.c

proctab.c: maketab

awk.g.o: awk.g.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(IWCHAR) $(ICOMMON) $(IUXRE) -c awk.g.c

awk.g.2001.o: awk.g.2001.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(IWCHAR) $(ICOMMON) $(IUXRE) -c awk.g.2001.c

awk.lx.o: awk.lx.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(IWCHAR) $(ICOMMON) $(IUXRE) -c awk.lx.c

b.o: b.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(IWCHAR) $(ICOMMON) $(IUXRE) -c b.c

lib.o: lib.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(IWCHAR) $(ICOMMON) $(IUXRE) -c lib.c

main.o: main.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(IWCHAR) $(ICOMMON) $(IUXRE) -c main.c

maketab.o: maketab.c
	$(HOSTCC) -DIN_MAKETAB -c maketab.c

parse.o: parse.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(IWCHAR) $(ICOMMON) $(IUXRE) -c parse.c

proctab.o: proctab.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(IWCHAR) $(ICOMMON) $(IUXRE) -c proctab.c

run.o: run.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(IWCHAR) $(ICOMMON) $(IUXRE) -c run.c

tran.o: tran.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(IWCHAR) $(ICOMMON) $(IUXRE) -c tran.c

version.o: version.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(IWCHAR) $(ICOMMON) $(IUXRE) -c version.c

version_sus.o: version.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(IWCHAR) $(ICOMMON) $(IUXRE) -DSUS -c version.c -o version_sus.o

version_su3.o: version.c
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(IWCHAR) $(ICOMMON) $(IUXRE) -DSU3 -c version.c -o version_su3.o

clean:
	rm -f awk.g.o awk.g.2001.c awk.g.2001.o awk.g.2001.y \
		awk.lx.o b.o lib.o main.o parse.o proctab.o run.o \
		tran.o version.o version_sus.o maketab.o maketab proctab.c \
		y.tab.c awk.g.c awk awk_sus awk.lx.c y.tab.h core log *~ \
		version_su3.o  awk_su3 awk_su3.o

install: all
	$(UCBINST) -c awk $(ROOT)$(SV3BIN)/nawk
	$(STRIP) $(ROOT)$(SV3BIN)/nawk
	$(UCBINST) -c awk_sus $(ROOT)$(SUSBIN)/nawk
	$(STRIP) $(ROOT)$(SUSBIN)/nawk
	$(UCBINST) -c awk_su3 $(ROOT)$(SU3BIN)/nawk
	$(STRIP) $(ROOT)$(SU3BIN)/nawk
	$(MANINST) -c -m 644 nawk.1 $(ROOT)$(MANDIR)/man1/nawk.1

awk.g.o: awk.h y.tab.h
awk.g.2001.o: awk.h y.tab.h
awk.lx.o: awk.h y.tab.h
b.o: awk.h y.tab.h
lib.o: awk.h y.tab.h
main.o: awk.h y.tab.h
maketab.o: awk.h y.tab.h
parse.o: awk.h y.tab.h
proctab.o: awk.h y.tab.h
run.o: awk.h y.tab.h
tran.o: awk.h y.tab.h
version.o: awk.h y.tab.h
