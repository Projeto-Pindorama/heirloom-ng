all: fmt

fmt: fmt.o
	$(LD) $(LDFLAGS) fmt.o $(LCOMMON) $(LWCHAR) $(LIBS) -o fmt

fmt.o: fmt.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO6FL) $(IWCHAR) $(ICOMMON) $(LARGEF) -c fmt.c

install: all
	$(UCBINST) -c fmt $(ROOT)$(DEFBIN)/fmt
	$(STRIP) $(ROOT)$(DEFBIN)/fmt
	$(MANINST) -c -m 644 fmt.1 $(ROOT)$(MANDIR)/man1/fmt.1

clean:
	rm -f fmt fmt.o core log *~
