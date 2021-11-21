all: pg pg_sus pg_su3

pg: pg.o
	$(LD) $(LDFLAGS) pg.o $(LCURS) $(LCOMMON) $(LWCHAR) $(LIBS) -o pg

pg.o: pg.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO6FL) $(LARGEF) $(IWCHAR) $(ICOMMON) -c pg.c

pg_sus: pg_sus.o
	$(LD) $(LDFLAGS) pg_sus.o $(LCURS) $(LCOMMON) $(LUXRE) $(LWCHAR) $(LIBS) -o pg_sus

pg_sus.o: pg.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO6FL) $(LARGEF) $(IWCHAR) $(ICOMMON) $(IUXRE) -DSUS -c pg.c -o pg_sus.o

pg_su3: pg_su3.o
	$(LD) $(LDFLAGS) pg_su3.o $(LCURS) $(LCOMMON) $(LUXRE) $(LWCHAR) $(LIBS) -o pg_su3

pg_su3.o: pg.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO6FL) $(LARGEF) $(IWCHAR) $(ICOMMON) $(IUXRE) -DSU3 -c pg.c -o pg_su3.o

install: all
	$(UCBINST) -c pg $(ROOT)$(SV3BIN)/pg
	$(STRIP) $(ROOT)$(SV3BIN)/pg
	$(UCBINST) -c pg_sus $(ROOT)$(SUSBIN)/pg
	$(STRIP) $(ROOT)$(SUSBIN)/pg
	$(UCBINST) -c pg_su3 $(ROOT)$(SU3BIN)/pg
	$(STRIP) $(ROOT)$(SU3BIN)/pg
	$(MANINST) -c -m 644 pg.1 $(ROOT)$(MANDIR)/man1/pg.1

clean:
	rm -f pg pg.o pg_sus pg_sus.o pg_su3 pg_su3.o core log *~
