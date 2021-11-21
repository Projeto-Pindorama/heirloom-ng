all: random

random: random.o
	$(LD) $(LDFLAGS) random.o $(LCOMMON) $(LIBS) -o random

random.o: random.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) -c random.c

install: all
	$(UCBINST) -c random $(ROOT)$(DEFBIN)/random
	$(STRIP) $(ROOT)$(DEFBIN)/random
	$(MANINST) -c -m 644 random.1 $(ROOT)$(MANDIR)/man1/random.1

clean:
	rm -f random random.o core log *~
