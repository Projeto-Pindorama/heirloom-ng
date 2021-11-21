all: nohup nohup_sus

nohup: nohup.o
	$(LD) $(LDFLAGS) nohup.o $(LCOMMON) $(LIBS) -o nohup

nohup.o: nohup.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) -c nohup.c

nohup_sus: nohup_sus.o
	$(LD) $(LDFLAGS) nohup_sus.o $(LCOMMON) $(LIBS) -o nohup_sus

nohup_sus.o: nohup.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) -DSUS -c nohup.c -o nohup_sus.o

install: all
	$(UCBINST) -c nohup $(ROOT)$(SV3BIN)/nohup
	$(STRIP) $(ROOT)$(SV3BIN)/nohup
	$(UCBINST) -c nohup_sus $(ROOT)$(SUSBIN)/nohup
	$(STRIP) $(ROOT)$(SUSBIN)/nohup
	$(MANINST) -c -m 644 nohup.1 $(ROOT)$(MANDIR)/man1/nohup.1

clean:
	rm -f nohup nohup.o nohup_sus nohup_sus.o core log *~
