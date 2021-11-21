all: touch touch_sus

touch: touch.o
	$(LD) $(LDFLAGS) touch.o $(LCOMMON) $(LIBS) -o touch

touch.o: touch.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LARGEF) -c touch.c

touch_sus: touch_sus.o
	$(LD) $(LDFLAGS) touch_sus.o $(LCOMMON) $(LIBS) -o touch_sus

touch_sus.o: touch.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LARGEF) -DSUS -c touch.c -o touch_sus.o

install: all
	$(UCBINST) -c touch $(ROOT)$(SV3BIN)/touch
	$(STRIP) $(ROOT)$(SV3BIN)/touch
	$(UCBINST) -c touch_sus $(ROOT)$(SUSBIN)/touch
	$(STRIP) $(ROOT)$(SUSBIN)/touch
	rm -f $(ROOT)$(DEFBIN)/settime
	$(LNS) touch $(ROOT)$(DEFBIN)/settime
	$(MANINST) -c -m 644 touch.1 $(ROOT)$(MANDIR)/man1/touch.1
	$(MANINST) -c -m 644 settime.1 $(ROOT)$(MANDIR)/man1/settime.1

clean:
	rm -f touch touch.o touch_sus touch_sus.o core log *~
