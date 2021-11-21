all: chmod chmod_sus

chmod: chmod.o
	$(LD) $(LDFLAGS) chmod.o $(LCOMMON) $(LIBS) -o chmod

chmod.o: chmod.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) -c chmod.c

chmod_sus: chmod_sus.o
	$(LD) $(LDFLAGS) chmod_sus.o $(LCOMMON) $(LIBS) -o chmod_sus

chmod_sus.o: chmod.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) -DSUS -c chmod.c -o chmod_sus.o

install: all
	$(UCBINST) -c chmod $(ROOT)$(SV3BIN)/chmod
	$(STRIP) $(ROOT)$(SV3BIN)/chmod
	$(UCBINST) -c chmod_sus $(ROOT)$(SUSBIN)/chmod
	$(STRIP) $(ROOT)$(SUSBIN)/chmod
	$(MANINST) -c -m 644 chmod.1 $(ROOT)$(MANDIR)/man1/chmod.1

clean:
	rm -f chmod chmod.o chmod_sus chmod_sus.o core log *~
