all: env

env: env.o
	$(LD) $(LDFLAGS) env.o $(LCOMMON) $(LIBS) -o env

env.o: env.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) -c env.c

install: all
	$(UCBINST) -c env $(ROOT)$(DEFBIN)/env
	$(STRIP) $(ROOT)$(DEFBIN)/env
	$(MANINST) -c -m 644 env.1 $(ROOT)$(MANDIR)/man1/env.1

clean:
	rm -f env env.o core log *~
