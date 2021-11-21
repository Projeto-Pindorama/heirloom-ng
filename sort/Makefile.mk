all: sort sort_sus

sort: sort.o
	$(LD) $(LDFLAGS) sort.o $(LCOMMON) $(LWCHAR) $(LIBS) -o sort

sort.o: sort.c
	$(CC) $(CFLAGS2) $(CPPFLAGS) $(XO6FL) $(LARGEF) $(IWCHAR) $(ICOMMON) -c sort.c

sort_sus: sort_sus.o
	$(LD) $(LDFLAGS) sort_sus.o $(LCOMMON) $(LWCHAR) $(LIBS) -o sort_sus

sort_sus.o: sort.c
	$(CC) $(CFLAGS2) $(CPPFLAGS) $(XO6FL) $(LARGEF) -DSUS $(IWCHAR) $(ICOMMON) -c sort.c -o sort_sus.o

install: all
	$(UCBINST) -c sort $(ROOT)$(SV3BIN)/sort
	$(STRIP) $(ROOT)$(SV3BIN)/sort
	$(UCBINST) -c sort_sus $(ROOT)$(SUSBIN)/sort
	$(STRIP) $(ROOT)$(SUSBIN)/sort
	$(MANINST) -c -m 644 sort.1 $(ROOT)$(MANDIR)/man1/sort.1

clean:
	rm -f sort sort.o sort_sus sort_sus.o core log *~
