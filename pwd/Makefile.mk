all: pwd

pwd: pwd.o
	$(LD) $(LDFLAGS) pwd.o $(LCOMMON) $(LIBS) -o pwd

pwd.o: pwd.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(ICOMMON) -c pwd.c

install: all
	$(UCBINST) -c pwd $(ROOT)$(DEFBIN)/pwd
	$(STRIP) $(ROOT)$(DEFBIN)/pwd
	$(MANINST) -c -m 644 pwd.1 $(ROOT)$(MANDIR)/man1/pwd.1

clean:
	rm -f pwd pwd.o core log *~
