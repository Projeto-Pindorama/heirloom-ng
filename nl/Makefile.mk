all: nl nl_sus nl_su3 nl_s42

nl: nl.o
	$(LD) $(LDFLAGS) nl.o $(LCOMMON) $(LWCHAR) $(LIBS) -o nl

nl.o: nl.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(IWCHAR) $(ICOMMON) $(LARGEF) -c nl.c

nl_sus: nl_sus.o
	$(LD) $(LDFLAGS) nl_sus.o $(LCOMMON) $(LUXRE) $(LWCHAR) $(LCOMMON) $(LIBS) -o nl_sus

nl_sus.o: nl.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(IWCHAR) $(ICOMMON) $(IUXRE) $(LARGEF) -DSUS -c nl.c -o nl_sus.o

nl_su3: nl_su3.o
	$(LD) $(LDFLAGS) nl_su3.o $(LCOMMON) $(LUXRE) $(LWCHAR) $(LCOMMON) $(LIBS) -o nl_su3

nl_su3.o: nl.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(IWCHAR) $(ICOMMON) $(IUXRE) $(LARGEF) -DSU3 -c nl.c -o nl_su3.o

nl_s42: nl_s42.o
	$(LD) $(LDFLAGS) nl_s42.o $(LCOMMON) $(LUXRE) $(LWCHAR) $(LCOMMON) $(LIBS) -o nl_s42

nl_s42.o: nl.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(IWCHAR) $(ICOMMON) $(IUXRE) $(LARGEF) -DS42 -c nl.c -o nl_s42.o

install: all
	$(UCBINST) -c nl $(ROOT)$(SV3BIN)/nl
	$(STRIP) $(ROOT)$(SV3BIN)/nl
	$(UCBINST) -c nl_s42 $(ROOT)$(S42BIN)/nl
	$(STRIP) $(ROOT)$(S42BIN)/nl
	$(UCBINST) -c nl_sus $(ROOT)$(SUSBIN)/nl
	$(STRIP) $(ROOT)$(SUSBIN)/nl
	$(UCBINST) -c nl_su3 $(ROOT)$(SU3BIN)/nl
	$(STRIP) $(ROOT)$(SU3BIN)/nl
	$(MANINST) -c -m 644 nl.1 $(ROOT)$(MANDIR)/man1/nl.1

clean:
	rm -f nl nl.o nl_sus nl_sus.o nl_su3 nl_su3.o nl_s42 nl_s42.o core log *~
