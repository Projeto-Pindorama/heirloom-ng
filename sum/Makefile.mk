all: sum sum_ucb

sum: sum.o
	$(LD) $(LDFLAGS) sum.o $(LCOMMON) $(LIBS) -o sum

sum.o: sum.c
	$(CC) $(CFLAGSU) $(CPPFLAGS) $(XO5FL) $(LARGEF) -c sum.c

sum_ucb: sum_ucb.o
	$(LD) $(LDFLAGS) sum_ucb.o $(LCOMMON) $(LIBS) -o sum_ucb

sum_ucb.o: sum.c
	$(CC) $(CFLAGSU) $(CPPFLAGS) $(XO5FL) $(LARGEF) -DUCB -c sum.c -o sum_ucb.o

install: all
	$(UCBINST) -c sum $(ROOT)$(DEFBIN)/sum
	$(STRIP) $(ROOT)$(DEFBIN)/sum
	$(UCBINST) -c sum_ucb $(ROOT)$(UCBBIN)/sum
	$(STRIP) $(ROOT)$(UCBBIN)/sum
	$(MANINST) -c -m 644 sum.1 $(ROOT)$(MANDIR)/man1/sum.1
	$(MANINST) -c -m 644 sum.1b $(ROOT)$(MANDIR)/man1b/sum.1b

clean:
	rm -f sum sum.o sum_ucb sum_ucb.o core log *~
