all: pr pr_sus

pr: pr.o
	$(LD) $(LDFLAGS) pr.o $(LCOMMON) $(LWCHAR) $(LIBS) -o pr

pr.o: pr.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LARGEF) $(XO6FL) $(IWCHAR) $(ICOMMON) -c pr.c

pr_sus: pr_sus.o
	$(LD) $(LDFLAGS) pr_sus.o $(LCOMMON) $(LWCHAR) $(LIBS) -o pr_sus

pr_sus.o: pr.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LARGEF) $(XO6FL) $(IWCHAR) $(ICOMMON) -DSUS -c pr.c -o pr_sus.o

install: all
	$(UCBINST) -c pr $(ROOT)$(SV3BIN)/pr
	$(STRIP) $(ROOT)$(SV3BIN)/pr
	$(UCBINST) -c pr_sus $(ROOT)$(SUSBIN)/pr
	$(STRIP) $(ROOT)$(SUSBIN)/pr
	$(MANINST) -c -m 644 pr.1 $(ROOT)$(MANDIR)/man1/pr.1

clean:
	rm -f pr pr.o pr_sus pr_sus.o core log *~
