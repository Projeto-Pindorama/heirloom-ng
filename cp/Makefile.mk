all: cp cp_s42 cp_sus ln ln_s42 ln_sus mv mv_s42 mv_sus

cp: cp.o
	$(LD) $(LDFLAGS) cp.o $(LCOMMON) $(LWCHAR) $(LIBS) $(LSOCKET) -o cp

cp.o: cp.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(IWCHAR) $(ICOMMON) $(LARGEF) $(GNUFL) -c cp.c

cp_s42: cp_s42.o
	$(LD) $(LDFLAGS) cp_s42.o $(LCOMMON) $(LWCHAR) $(LIBS) $(LSOCKET) -o cp_s42

cp_s42.o: cp.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(IWCHAR) $(ICOMMON) $(LARGEF) $(GNUFL) -DS42 -c cp.c -o cp_s42.o

cp_sus: cp_sus.o
	$(LD) $(LDFLAGS) cp_sus.o $(LCOMMON) $(LWCHAR) $(LIBS) $(LSOCKET) -o cp_sus

cp_sus.o: cp.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(IWCHAR) $(ICOMMON) $(LARGEF) $(GNUFL) -DSUS -c cp.c -o cp_sus.o

ln: cp
	rm -f ln
	$(LNS) cp ln

ln_s42: cp_s42
	rm -f ln_s42
	$(LNS) cp_s42 ln_s42

ln_sus: cp_sus
	rm -f ln_sus
	$(LNS) cp_sus ln_sus

mv: cp
	rm -f mv
	$(LNS) cp mv

mv_s42: cp_s42
	rm -f mv_s42
	$(LNS) cp_s42 mv_s42

mv_sus: cp_sus
	rm -f mv_sus
	$(LNS) cp_sus mv_sus

install: all
	$(UCBINST) -c cp $(ROOT)$(SV3BIN)/cp
	$(STRIP) $(ROOT)$(SV3BIN)/cp
	rm -f $(ROOT)$(SV3BIN)/ln $(ROOT)$(SV3BIN)/mv
	$(LNS) cp $(ROOT)$(SV3BIN)/ln
	$(LNS) cp $(ROOT)$(SV3BIN)/mv
	$(UCBINST) -c cp_s42 $(ROOT)$(S42BIN)/cp
	$(STRIP) $(ROOT)$(S42BIN)/cp
	rm -f $(ROOT)$(S42BIN)/ln $(ROOT)$(S42BIN)/mv
	$(LNS) cp $(ROOT)$(S42BIN)/ln
	$(LNS) cp $(ROOT)$(S42BIN)/mv
	$(UCBINST) -c cp_sus $(ROOT)$(SUSBIN)/cp
	$(STRIP) $(ROOT)$(SUSBIN)/cp
	rm -f $(ROOT)$(SUSBIN)/ln $(ROOT)$(SUSBIN)/mv
	$(LNS) cp $(ROOT)$(SUSBIN)/ln
	$(LNS) cp $(ROOT)$(SUSBIN)/mv
	$(MANINST) -c -m 644 cp.1 $(ROOT)$(MANDIR)/man1/cp.1
	$(MANINST) -c -m 644 ln.1 $(ROOT)$(MANDIR)/man1/ln.1
	$(MANINST) -c -m 644 mv.1 $(ROOT)$(MANDIR)/man1/mv.1

clean:
	rm -f cp cp.o cp_s42 cp_s42.o cp_sus cp_sus.o \
		ln ln_s42 ln_sus mv mv_s42 mv_sus core log *~
