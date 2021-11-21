all: mkdir mkdir_sus

mkdir: mkdir.o
	$(LD) $(LDFLAGS) mkdir.o $(LCOMMON) $(LIBS) -o mkdir

mkdir.o: mkdir.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) -c mkdir.c

mkdir_sus: mkdir_sus.o
	$(LD) $(LDFLAGS) mkdir_sus.o $(LCOMMON) $(LIBS) -o mkdir_sus

mkdir_sus.o: mkdir.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) -DSUS -c mkdir.c -o mkdir_sus.o

install: all
	$(UCBINST) -c mkdir $(ROOT)$(SV3BIN)/mkdir
	$(STRIP) $(ROOT)$(SV3BIN)/mkdir
	$(UCBINST) -c mkdir_sus $(ROOT)$(SUSBIN)/mkdir
	$(STRIP) $(ROOT)$(SUSBIN)/mkdir
	$(MANINST) -c -m 644 mkdir.1 $(ROOT)$(MANDIR)/man1/mkdir.1

clean:
	rm -f mkdir mkdir.o mkdir_sus mkdir_sus.o core log *~
