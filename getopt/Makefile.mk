all: getopt

getopt: getopt.o
	$(LD) $(LDFLAGS) getopt.o $(LCOMMON) $(LIBS) -o getopt

getopt.o: getopt.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(ICOMMON) -c getopt.c

install: all
	$(UCBINST) -c getopt $(ROOT)$(DEFBIN)/getopt
	$(STRIP) $(ROOT)$(DEFBIN)/getopt
	$(MANINST) -c -m 644 getopt.1 $(ROOT)$(MANDIR)/man1/getopt.1

clean:
	rm -f getopt getopt.o core log *~
