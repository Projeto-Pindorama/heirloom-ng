all: dc

dc: dc.o version.o
	$(LD) $(LDFLAGS) dc.o version.o $(LCOMMON) $(LWCHAR) $(LIBS) -o dc

dc.o: dc.c dc.h
	$(CC) $(CFLAGS2) $(CPPFLAGS) $(XO5FL) $(IWCHAR) $(ICOMMON) -DSHELL='"$(SHELL)"' -c dc.c

version.o: version.c
	$(CC) $(CFLAGS2) $(CPPFLAGS) $(XO5FL) $(IWCHAR) $(ICOMMON) -c version.c

install: all
	$(UCBINST) -c dc $(ROOT)$(DEFBIN)/dc
	$(STRIP) $(ROOT)$(DEFBIN)/dc
	$(MANINST) -c -m 644 dc.1 $(ROOT)$(MANDIR)/man1/dc.1

clean:
	rm -f dc dc.o version.o core log *~
