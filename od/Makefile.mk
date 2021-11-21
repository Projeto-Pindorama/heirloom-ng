all: od od_sus

od: od.o
	$(LD) $(LDFLAGS) od.o $(LCOMMON) $(LWCHAR) $(LIBS) -o od

od.o: od.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO6FL) $(LARGEF) $(IWCHAR) $(ICOMMON) -c od.c

od_sus: od_sus.o
	$(LD) $(LDFLAGS) od_sus.o $(LCOMMON) $(LWCHAR) $(LIBS) -o od_sus

od_sus.o: od.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO6FL) $(LARGEF) $(IWCHAR) $(ICOMMON) -DSUS -c od.c -o od_sus.o

install: all
	$(UCBINST) -c od $(ROOT)$(SV3BIN)/od
	$(STRIP) $(ROOT)$(SV3BIN)/od
	$(UCBINST) -c od $(ROOT)$(SUSBIN)/od
	$(STRIP) $(ROOT)$(SUSBIN)/od
	$(MANINST) -c -m 644 od.1 $(ROOT)$(MANDIR)/man1/od.1

clean:
	rm -f od od.o od od_sus od_sus.o core log *~
