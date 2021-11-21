all: echo echo_sus echo_ucb

echo: echo.o main.o version.o
	$(LD) $(LDFLAGS) echo.o main.o version.o $(LCOMMON) $(LWCHAR) $(LIBS) -o echo

echo_sus: echo_sus.o main.o version_sus.o
	$(LD) $(LDFLAGS) echo_sus.o main.o version_sus.o $(LCOMMON) $(LWCHAR) $(LIBS) -o echo_sus

echo_ucb: echo_ucb.o main.o version_ucb.o
	$(LD) $(LDFLAGS) echo_ucb.o main.o version_ucb.o $(LCOMMON) $(LWCHAR) $(LIBS) -o echo_ucb

echo.o: echo.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(ICOMMON) $(IWCHAR) -c echo.c

echo_sus.o: echo.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(ICOMMON) $(IWCHAR) -DSUS -c echo.c -o echo_sus.o

echo_ucb.o: echo.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(ICOMMON) $(IWCHAR) -DUCB -c echo.c -o echo_ucb.o

main.o: main.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(ICOMMON) $(IWCHAR) -Dfunc='echo' -c main.c

version.o: version.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(ICOMMON) -c version.c

version_sus.o: version.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(ICOMMON) -DSUS -c version.c -o version_sus.o

version_ucb.o: version.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(ICOMMON) -DUCB -c version.c -o version_ucb.o

install: all
	$(UCBINST) -c echo $(ROOT)$(SV3BIN)/echo
	$(STRIP) $(ROOT)$(SV3BIN)/echo
	$(UCBINST) -c echo_sus $(ROOT)$(SUSBIN)/echo
	$(STRIP) $(ROOT)$(SUSBIN)/echo
	$(UCBINST) -c echo_ucb $(ROOT)$(UCBBIN)/echo
	$(STRIP) $(ROOT)$(UCBBIN)/echo
	$(MANINST) -c -m 644 echo.1 $(ROOT)$(MANDIR)/man1/echo.1
	$(MANINST) -c -m 644 echo.1b $(ROOT)$(MANDIR)/man1b/echo.1b

clean:
	rm -f echo echo.o main.o version.o echo_sus echo_sus.o version_sus.o echo_ucb echo_ucb.o version_ucb.o core log *~

main.o: main.c defs.h
echo.o: echo.c defs.h
echo_sus.o: echo.c defs.h
echo_ucb.o: echo.c defs.h
