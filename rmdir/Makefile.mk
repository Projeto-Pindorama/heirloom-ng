all: rmdir rmdir_sus

rmdir: rmdir.o
	$(LD) $(LDFLAGS) rmdir.o $(LCOMMON) $(LIBS) -o rmdir

rmdir.o: rmdir.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) -c rmdir.c

rmdir_sus: rmdir_sus.o
	$(LD) $(LDFLAGS) rmdir_sus.o $(LCOMMON) $(LIBS) -o rmdir_sus

rmdir_sus.o: rmdir.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) -DSUS -c rmdir.c -o rmdir_sus.o

install: all
	$(UCBINST) -c rmdir $(ROOT)$(SV3BIN)/rmdir
	$(STRIP) $(ROOT)$(SV3BIN)/rmdir
	$(UCBINST) -c rmdir_sus $(ROOT)$(SUSBIN)/rmdir
	$(STRIP) $(ROOT)$(SUSBIN)/rmdir
	$(MANINST) -c -m 644 rmdir.1 $(ROOT)$(MANDIR)/man1/rmdir.1

clean:
	rm -f rmdir rmdir.o rmdir_sus rmdir_sus.o core log *~
