all: csplit csplit_sus csplit_su3

csplit: csplit.o
	$(LD) $(LDFLAGS) csplit.o $(LCOMMON) $(LWCHAR) $(LIBS) -lm -o csplit

csplit.o: csplit.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO6FL) $(LARGEF) $(IWCHAR) $(ICOMMON) -c csplit.c

csplit_sus: csplit_sus.o
	$(LD) $(LDFLAGS) csplit_sus.o $(LUXRE) $(LCOMMON) $(LWCHAR) $(LIBS) -lm -o csplit_sus

csplit_sus.o: csplit.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO6FL) $(LARGEF) $(IUXRE) $(IWCHAR) $(ICOMMON) -DSUS -c csplit.c -o csplit_sus.o

csplit_su3: csplit_su3.o
	$(LD) $(LDFLAGS) csplit_su3.o $(LUXRE) $(LCOMMON) $(LWCHAR) $(LIBS) -lm -o csplit_su3

csplit_su3.o: csplit.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO6FL) $(LARGEF) $(IUXRE) $(IWCHAR) $(ICOMMON) -DSU3 -c csplit.c -o csplit_su3.o

install: all
	$(UCBINST) -c csplit $(ROOT)$(SV3BIN)/csplit
	$(STRIP) $(ROOT)$(SV3BIN)/csplit
	$(UCBINST) -c csplit_sus $(ROOT)$(SUSBIN)/csplit
	$(STRIP) $(ROOT)$(SUSBIN)/csplit
	$(UCBINST) -c csplit_su3 $(ROOT)$(SU3BIN)/csplit
	$(STRIP) $(ROOT)$(SU3BIN)/csplit
	$(MANINST) -c -m 644 csplit.1 $(ROOT)$(MANDIR)/man1/csplit.1

clean:
	rm -f csplit csplit.o csplit_sus csplit_sus.o \
		csplit_su3 csplit_su3.o core log *~
