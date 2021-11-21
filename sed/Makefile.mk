all: sed sed_sus sed_su3 sed_s42

sed: sed0.o sed1.o version.o
	$(LD) $(LDFLAGS) sed0.o sed1.o version.o $(LCOMMON) $(LWCHAR) $(LIBS) -o sed

sed0.o: sed0.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(IWCHAR) $(ICOMMON) $(LARGEF) -c sed0.c

sed1.o: sed1.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(IWCHAR) $(ICOMMON) $(LARGEF) -c sed1.c

version.o: version.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(IWCHAR) $(ICOMMON) $(LARGEF) -c version.c

sed_sus: sed0_sus.o sed1_sus.o version_sus.o
	$(LD) $(LDFLAGS) sed0_sus.o sed1_sus.o version_sus.o $(LUXRE) $(LCOMMON) $(LWCHAR) $(LIBS) -o sed_sus

sed0_sus.o: sed0.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(IUXRE) $(IWCHAR) $(LARGEF) -DSUS -c sed0.c -o sed0_sus.o

sed1_sus.o: sed1.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(IUXRE) $(IWCHAR) $(LARGEF) -DSUS -c sed1.c -o sed1_sus.o

version_sus.o: version.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(IUXRE) $(IWCHAR) $(LARGEF) -DSUS -c version.c -o version_sus.o

sed_su3: sed0_su3.o sed1_su3.o version_su3.o
	$(LD) $(LDFLAGS) sed0_su3.o sed1_su3.o version_su3.o $(LUXRE) $(LCOMMON) $(LWCHAR) $(LIBS) -o sed_su3

sed0_su3.o: sed0.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(IUXRE) $(IWCHAR) $(LARGEF) -DSU3 -c sed0.c -o sed0_su3.o

sed1_su3.o: sed1.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(IUXRE) $(IWCHAR) $(LARGEF) -DSU3 -c sed1.c -o sed1_su3.o

version_su3.o: version.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(IUXRE) $(IWCHAR) $(LARGEF) -DSU3 -c version.c -o version_su3.o

sed_s42: sed0_s42.o sed1_s42.o version_s42.o
	$(LD) $(LDFLAGS) sed0_s42.o sed1_s42.o version_s42.o $(LUXRE) $(LCOMMON) $(LWCHAR) $(LIBS) -o sed_s42

sed0_s42.o: sed0.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(IUXRE) $(IWCHAR) $(LARGEF) -DS42 -c sed0.c -o sed0_s42.o

sed1_s42.o: sed1.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(IUXRE) $(IWCHAR) $(LARGEF) -DS42 -c sed1.c -o sed1_s42.o

version_s42.o: version.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(IUXRE) $(IWCHAR) $(LARGEF) -DS42 -c version.c -o version_s42.o

install: all
	$(UCBINST) -c sed $(ROOT)$(SV3BIN)/sed
	$(STRIP) $(ROOT)$(SV3BIN)/sed
	$(UCBINST) -c sed_sus $(ROOT)$(SUSBIN)/sed
	$(STRIP) $(ROOT)$(SUSBIN)/sed
	$(UCBINST) -c sed_su3 $(ROOT)$(SU3BIN)/sed
	$(STRIP) $(ROOT)$(SU3BIN)/sed
	$(UCBINST) -c sed_s42 $(ROOT)$(S42BIN)/sed
	$(STRIP) $(ROOT)$(S42BIN)/sed
	$(MANINST) -c -m 644 sed.1 $(ROOT)$(MANDIR)/man1/sed.1

clean:
	rm -f sed sed0.o sed1.o version.o \
		sed_sus sed0_sus.o sed1_sus.o version_sus.o \
		sed_su3 sed0_su3.o sed1_su3.o version_su3.o \
		sed_s42 sed0_s42.o sed1_s42.o version_s42.o \
		core log *~
