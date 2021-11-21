all: wc wc_sus wc_s42

wc: wc.o
	$(LD) $(LDFLAGS) wc.o $(LCOMMON) $(LWCHAR) $(LIBS) -o wc

wc.o: wc.c
	$(CC) $(CFLAGS2) $(CPPFLAGS) $(XO6FL) $(LARGEF) $(IWCHAR) $(ICOMMON) -c wc.c

wc_sus: wc_sus.o
	$(LD) $(LDFLAGS) wc_sus.o $(LCOMMON) $(LWCHAR) $(LIBS) -o wc_sus

wc_sus.o: wc.c
	$(CC) $(CFLAGS2) $(CPPFLAGS) $(XO6FL) $(LARGEF) $(IWCHAR) $(ICOMMON) -DSUS -c wc.c -o wc_sus.o

wc_s42: wc_s42.o
	$(LD) $(LDFLAGS) wc_s42.o $(LCOMMON) $(LWCHAR) $(LIBS) -o wc_s42

wc_s42.o: wc.c
	$(CC) $(CFLAGS2) $(CPPFLAGS) $(XO6FL) $(LARGEF) $(IWCHAR) $(ICOMMON) -DS42 -c wc.c -o wc_s42.o

install: all
	$(UCBINST) -c wc $(ROOT)$(SV3BIN)/wc
	$(STRIP) $(ROOT)$(SV3BIN)/wc
	$(UCBINST) -c wc_sus $(ROOT)$(SUSBIN)/wc
	$(STRIP) $(ROOT)$(SUSBIN)/wc
	$(UCBINST) -c wc_s42 $(ROOT)$(S42BIN)/wc
	$(STRIP) $(ROOT)$(S42BIN)/wc
	$(MANINST) -c -m 644 wc.1 $(ROOT)$(MANDIR)/man1/wc.1

clean:
	rm -f wc wc.o wc_sus wc_sus.o wc_s42 wc_s42.o core log *~
