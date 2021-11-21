all: ps ps_sus ps_s42 ps_ucb

ps: ps.o
	$(LD) $(LDFLAGS) ps.o $(LCOMMON) $(LWCHAR) $(LIBGEN) $(LIBS) $(LKVM) -o ps

ps.o: ps.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(GNUFL) $(IWCHAR) $(ICOMMON) -DUSE_PS_CACHE -DDEFAULT='"$(DFLDIR)/ps"' -c ps.c

ps_sus: ps_sus.o
	$(LD) $(LDFLAGS) ps_sus.o $(LCOMMON) $(LWCHAR) $(LIBGEN) $(LIBS) $(LKVM) -o ps_sus

ps_sus.o: ps.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(GNUFL) $(IWCHAR) $(ICOMMON) -DUSE_PS_CACHE -DDEFAULT='"$(DFLDIR)/ps"' -DSUS -c ps.c -o ps_sus.o

ps_s42: ps_s42.o
	$(LD) $(LDFLAGS) ps_s42.o $(LCOMMON) $(LWCHAR) $(LIBGEN) $(LIBS) $(LKVM) -o ps_s42

ps_s42.o: ps.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(GNUFL) $(IWCHAR) $(ICOMMON) -DUSE_PS_CACHE -DDEFAULT='"$(DFLDIR)/ps"' -DS42 -c ps.c -o ps_s42.o

ps_ucb: ps_ucb.o
	$(LD) $(LDFLAGS) ps_ucb.o $(LCOMMON) $(LWCHAR) $(LIBGEN) $(LIBS) $(LKVM) -o ps_ucb

ps_ucb.o: ps.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(GNUFL) $(IWCHAR) $(ICOMMON) -DUSE_PS_CACHE -DDEFAULT='"$(DFLDIR)/ps"' -DUCB -c ps.c -o ps_ucb.o

install: all
	$(UCBINST) -c -m 4755 ps $(ROOT)$(SV3BIN)/ps
	$(STRIP) $(ROOT)$(SV3BIN)/ps
	$(UCBINST) -c -m 4755 ps_s42 $(ROOT)$(S42BIN)/ps
	$(STRIP) $(ROOT)$(S42BIN)/ps
	$(UCBINST) -c -m 4755 ps_sus $(ROOT)$(SUSBIN)/ps
	$(STRIP) $(ROOT)$(SUSBIN)/ps
	$(UCBINST) -c -m 4755 ps_ucb $(ROOT)$(UCBBIN)/ps
	$(STRIP) $(ROOT)$(UCBBIN)/ps
	$(MANINST) -c -m 644 ps.1 $(ROOT)$(MANDIR)/man1/ps.1
	$(MANINST) -c -m 644 ps.1b $(ROOT)$(MANDIR)/man1b/ps.1b
	test -r $(ROOT)$(DFLDIR)/ps || $(UCBINST) -c -m 644 ps.dfl $(ROOT)$(DFLDIR)/ps

clean:
	rm -f ps ps.o ps_sus ps_sus.o ps_s42 ps_s42.o ps_ucb ps_ucb.o core log *~
