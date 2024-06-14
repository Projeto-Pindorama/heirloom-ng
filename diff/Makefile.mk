all: diff diffh

diff: diff.o diffdir.o diffreg.o diffver.o
	$(LD) $(LDFLAGS) diff.o diffdir.o diffreg.o diffver.o $(LCOMMON) $(LWCHAR) $(LIBS) -o diff

diff.o: diff.c
	$(CC) $(CFLAGS2) $(CPPFLAGS) $(XO6FL) $(LARGEF) $(IWCHAR) $(ICOMMON) -DDIFFH='"$(DEFLIB)/diffh"' -c diff.c

diffdir.o: diffdir.c
	$(CC) $(CFLAGS2) $(CPPFLAGS) $(XO6FL) $(LARGEF) $(IWCHAR) $(ICOMMON) -c diffdir.c

diffreg.o: diffreg.c
	$(CC) $(CFLAGS2) $(CPPFLAGS) $(XO6FL) $(LARGEF) $(IWCHAR) $(ICOMMON) -c diffreg.c

diffver.o: diffver.c
	$(CC) $(CFLAGS2) $(CPPFLAGS) $(XO6FL) $(LARGEF) $(IWCHAR) $(ICOMMON) -c diffver.c

diffh: diffh.o
	$(LD) $(LDFLAGS) diffh.o $(LCOMMON) $(LWCHAR) $(LIBS) -o diffh

diffh.o: diffh.c
	$(CC) $(CFLAGS2) $(CPPFLAGS) $(XO6FL) $(LARGEF) $(IWCHAR) $(ICOMMON) -c diffh.c

install: all
	$(UCBINST) -c diff $(ROOT)$(DEFBIN)/diff
	$(STRIP) $(ROOT)$(DEFBIN)/diff
	$(UCBINST) -c diffh $(ROOT)$(DEFLIB)/diffh
	$(STRIP) $(ROOT)$(DEFLIB)/diffh
	$(MANINST) -c -m 644 diff.1 $(ROOT)$(MANDIR)/man1/diff.1

clean:
	rm -f diff diff.o diffdir.o diffreg.o diffver.o diffh diffh.o core log *~

diff.o: diff.h
diffdir.o: diff.h
diffreg.o: diff.h
