all: df df_ucb dfspace

df: df.o
	$(LD) $(LDFLAGS) df.o $(LCOMMON) $(LIBS) $(LIBGEN) -o df

df.o: df.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) -c df.c

df_ucb: df_ucb.o
	$(LD) $(LDFLAGS) df_ucb.o $(LCOMMON) $(LIBS) $(LIBGEN) -o df_ucb

df_ucb.o: df.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) -DUCB -c df.c -o df_ucb.o

dfspace: df
	rm -f dfspace
	$(LNS) df dfspace

install: all
	$(UCBINST) -c df $(ROOT)$(DEFBIN)/df
	$(STRIP) $(ROOT)$(DEFBIN)/df
	rm -f $(ROOT)$(DEFBIN)/dfspace
	$(LNS) df $(ROOT)$(DEFBIN)/dfspace
	$(UCBINST) -c df_ucb $(ROOT)$(UCBBIN)/df
	$(STRIP) $(ROOT)$(UCBBIN)/df
	$(MANINST) -c -m 644 df.1 $(ROOT)$(MANDIR)/man1/df.1
	rm -f $(ROOT)$(MANDIR)/man1/dfspace.1
	$(LNS) df.1 $(ROOT)$(MANDIR)/man1/dfspace.1
	$(MANINST) -c -m 644 df.1b $(ROOT)$(MANDIR)/man1b/df.1b

clean:
	rm -f df df.o df_ucb df_ucb.o dfspace core log *~
