all: logname

logname: logname.o
	$(LD) $(LDFLAGS) logname.o $(LCOMMON) $(LWCHAR) $(LIBS) -o logname

logname.o: logname.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(IWCHAR) $(ICOMMON) -c logname.c

install: all
	$(UCBINST) -c logname $(ROOT)$(DEFBIN)/logname
	$(STRIP) $(ROOT)$(DEFBIN)/logname
	$(MANINST) -c -m 644 logname.1 $(ROOT)$(MANDIR)/man1/logname.1

clean:
	rm -f logname logname.o core log *~
