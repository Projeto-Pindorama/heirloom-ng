all: join

join: join.o
	$(LD) $(LDFLAGS) join.o $(LCOMMON) $(LWCHAR) $(LIBS) -o join

join.o: join.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO6FL) $(LARGEF) $(IWCHAR) $(ICOMMON) -c join.c

install: all
	$(UCBINST) -c join $(ROOT)$(DEFBIN)/join
	$(STRIP) $(ROOT)$(DEFBIN)/join
	$(MANINST) -c -m 644 join.1 $(ROOT)$(MANDIR)/man1/join.1

clean:
	rm -f join join.o core log *~
