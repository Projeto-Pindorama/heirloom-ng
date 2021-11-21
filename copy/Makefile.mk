all: copy

copy: copy.o
	$(LD) $(LDFLAGS) copy.o $(LCOMMON) $(LWCHAR) $(LIBS) -o copy

copy.o: copy.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO6FL) $(LARGEF) $(IWCHAR) $(ICOMMON) -c copy.c

install: all
	$(UCBINST) -c copy $(ROOT)$(DEFBIN)/copy
	$(STRIP) $(ROOT)$(DEFBIN)/copy
	$(MANINST) -c -m 644 copy.1 $(ROOT)$(MANDIR)/man1/copy.1

clean:
	rm -f copy copy.o core log *~
