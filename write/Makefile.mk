all: write 

write: write.o
	$(LD) $(LDFLAGS) write.o $(LCOMMON) $(LWCHAR) $(LIBS) -o write

write.o: write.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(ICOMMON) -DSHELL='"$(SHELL)"' -std=c89 -c write.c

install: all
	$(UCBINST) -c write $(ROOT)$(DEFBIN)/write
	$(STRIP) $(ROOT)$(DEFBIN)/write
	$(MANINST) -c -m 644 write.1 $(ROOT)$(MANDIR)/man1/write.1

clean:
	rm -f write write.o core log *~
