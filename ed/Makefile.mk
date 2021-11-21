all: ed ed_s42 ed_sus ed_su3

ed: ed.o
	$(LD) $(LDFLAGS) ed.o $(LCOMMON) $(LWCHAR) $(LIBS) -o ed

ed.o: ed.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(IWCHAR) $(ICOMMON) -DSHELL='"$(SHELL)"' -c ed.c

ed_s42: ed_s42.o
	$(LD) $(LDFLAGS) ed_s42.o $(LCOMMON) $(LUXRE) $(LWCHAR) $(LIBS) -o ed_s42

ed_s42.o: ed.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(IWCHAR) $(ICOMMON) $(IUXRE) -DS42 -DSHELL='"$(SHELL)"' -c ed.c -o ed_s42.o

ed_sus: ed_sus.o
	$(LD) $(LDFLAGS) ed_sus.o $(LCOMMON) $(LUXRE) $(LWCHAR) $(LIBS) -o ed_sus

ed_sus.o: ed.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(IWCHAR) $(ICOMMON) $(IUXRE) -DSUS -DSHELL='"$(POSIX_SHELL)"' -c ed.c -o ed_sus.o

ed_su3: ed_su3.o
	$(LD) $(LDFLAGS) ed_su3.o $(LCOMMON) $(LUXRE) $(LWCHAR) $(LIBS) -o ed_su3

ed_su3.o: ed.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(IWCHAR) $(ICOMMON) $(IUXRE) -DSU3 -DSHELL='"$(POSIX_SHELL)"' -c ed.c -o ed_su3.o

install: all
	$(UCBINST) -c ed $(ROOT)$(SV3BIN)/ed
	$(STRIP) $(ROOT)$(SV3BIN)/ed
	$(UCBINST) -c ed_s42 $(ROOT)$(S42BIN)/ed
	$(STRIP) $(ROOT)$(S42BIN)/ed
	$(UCBINST) -c ed_sus $(ROOT)$(SUSBIN)/ed
	$(STRIP) $(ROOT)$(SUSBIN)/ed
	$(UCBINST) -c ed_su3 $(ROOT)$(SU3BIN)/ed
	$(STRIP) $(ROOT)$(SU3BIN)/ed
	$(MANINST) -c -m 644 ed.1 $(ROOT)$(MANDIR)/man1/ed.1

clean:
	rm -f ed ed.o ed_s42 ed_s42.o ed_sus ed_sus.o ed_su3 ed_su3.o core log *~
