all: id id_sus

id: id.o
	$(LD) $(LDFLAGS) id.o $(LCOMMON) $(LIBS) -o id

id.o: id.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) -c id.c

id_sus: id_sus.o
	$(LD) $(LDFLAGS) id_sus.o $(LCOMMON) $(LIBS) -o id_sus

id_sus.o: id.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) -DSUS -c id.c -o id_sus.o

install: all
	$(UCBINST) -c id $(ROOT)$(SV3BIN)/id
	$(STRIP) $(ROOT)$(SV3BIN)/id
	$(UCBINST) -c id_sus $(ROOT)$(SUSBIN)/id
	$(STRIP) $(ROOT)$(SUSBIN)/id
	$(MANINST) -c -m 644 id.1 $(ROOT)$(MANDIR)/man1/id.1

clean:
	rm -f id id.o id_sus id_sus.o core log *~
