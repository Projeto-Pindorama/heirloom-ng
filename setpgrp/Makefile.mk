all: setpgrp

setpgrp: setpgrp.o
	$(LD) $(LDFLAGS) setpgrp.o $(LCOMMON) $(LIBS) -o setpgrp

setpgrp.o: setpgrp.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) -c setpgrp.c

install: all
	$(UCBINST) -c setpgrp $(ROOT)$(DEFBIN)/setpgrp
	$(STRIP) $(ROOT)$(DEFBIN)/setpgrp
	$(MANINST) -c -m 644 setpgrp.1 $(ROOT)$(MANDIR)/man1/setpgrp.1

clean:
	rm -f setpgrp setpgrp.o core log *~
