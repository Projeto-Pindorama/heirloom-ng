all: file file_sus

file: file.o
	$(LD) $(LDFLAGS) file.o $(LCOMMON) $(LWCHAR) $(LIBS) -o file

file.o: file.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(GNUFL) $(LARGEF) $(IWCHAR) $(ICOMMON) -DMAGIC='"$(MAGIC)"' -c file.c

file_sus: file_sus.o
	$(LD) $(LDFLAGS) file_sus.o $(LCOMMON) $(LWCHAR) $(LIBS) -o file_sus

file_sus.o: file.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(GNUFL) $(LARGEF) $(IWCHAR) $(ICOMMON) -DSUS -DMAGIC='"$(MAGIC)"' -c file.c -o file_sus.o

install: all
	$(UCBINST) -c file $(ROOT)$(SV3BIN)/file
	$(STRIP) $(ROOT)$(SV3BIN)/file
	$(UCBINST) -c file_sus $(ROOT)$(SUSBIN)/file
	$(STRIP) $(ROOT)$(SUSBIN)/file
	$(UCBINST) -c -m 644 magic $(ROOT)$(MAGIC)
	$(MANINST) -c -m 644 file.1 $(ROOT)$(MANDIR)/man1/file.1

clean:
	rm -f file file.o file_sus file_sus.o core log *~
