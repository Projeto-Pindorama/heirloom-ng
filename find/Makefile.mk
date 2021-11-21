all: find find_sus find_su3

find: find.o
	$(LD) $(LDFLAGS) find.o $(LCOMMON) $(LWCHAR) $(LIBS) -o find

find.o: find.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LARGEF) $(IWCHAR) $(ICOMMON) $(GNUFL) -DGETDIR -c find.c

find_sus: find_sus.o
	$(LD) $(LDFLAGS) find_sus.o $(LCOMMON) $(LWCHAR) $(LIBS) -o find_sus

find_sus.o: find.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LARGEF) $(IWCHAR) $(ICOMMON) $(GNUFL) -DGETDIR -DSUS -c find.c -o find_sus.o

find_su3: find_su3.o
	$(LD) $(LDFLAGS) find_su3.o $(LCOMMON) $(LWCHAR) $(LIBS) -o find_su3

find_su3.o: find.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LARGEF) $(IWCHAR) $(ICOMMON) $(GNUFL) -DGETDIR -DSU3 -c find.c -o find_su3.o

install: all
	$(UCBINST) -c find $(ROOT)$(SV3BIN)/find
	$(STRIP) $(ROOT)$(SV3BIN)/find
	$(UCBINST) -c find_sus $(ROOT)$(SUSBIN)/find
	$(STRIP) $(ROOT)$(SUSBIN)/find
	$(UCBINST) -c find_su3 $(ROOT)$(SU3BIN)/find
	$(STRIP) $(ROOT)$(SU3BIN)/find
	$(MANINST) -c -m 644 find.1 $(ROOT)$(MANDIR)/man1/find.1

clean:
	rm -f find find.o find_sus find_sus.o find_su3 find_su3.o core log *~
