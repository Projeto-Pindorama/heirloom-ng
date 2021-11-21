all: test test_sus test_ucb

test: test.o main.o helper.o version.o
	$(LD) $(LDFLAGS) test.o main.o helper.o version.o $(LCOMMON) $(LWCHAR) $(LIBS) -o test

test_sus: test_sus.o main.o helper_sus.o version_sus.o
	$(LD) $(LDFLAGS) test_sus.o main.o helper_sus.o version_sus.o $(LCOMMON) $(LWCHAR) $(LIBS) -o test_sus

test_ucb: test_ucb.o main.o helper.o version_ucb.o
	$(LD) $(LDFLAGS) test_ucb.o main.o helper.o version_ucb.o $(LCOMMON) $(LWCHAR) $(LIBS) -o test_ucb

test.o: test.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(ICOMMON) $(IWCHAR) -c test.c

test_sus.o: test.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(ICOMMON) $(IWCHAR) -DSUS -c test.c -o test_sus.o

test_ucb.o: test.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(ICOMMON) $(IWCHAR) -DUCB -c test.c -o test_ucb.o

main.o: main.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(ICOMMON) $(IWCHAR) -Dfunc='test' -c main.c

helper.o: helper.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(ICOMMON) $(IWCHAR) -c helper.c

helper_sus.o: helper.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(ICOMMON) $(IWCHAR) -DSUS -c helper.c -o helper_sus.o

version.o: version.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(ICOMMON) -c version.c

version_sus.o: version.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(ICOMMON) -DSUS -c version.c -o version_sus.o

version_ucb.o: version.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(LARGEF) $(ICOMMON) -DUCB -c version.c -o version_ucb.o

install: all
	$(UCBINST) -c test $(ROOT)$(SV3BIN)/test
	$(STRIP) $(ROOT)$(SV3BIN)/test
	$(UCBINST) -c test_sus $(ROOT)$(SUSBIN)/test
	$(STRIP) $(ROOT)$(SUSBIN)/test
	$(UCBINST) -c test_ucb $(ROOT)$(UCBBIN)/test
	$(STRIP) $(ROOT)$(UCBBIN)/test
	$(MANINST) -c -m 644 test.1 $(ROOT)$(MANDIR)/man1/test.1
	$(MANINST) -c -m 644 test.1b $(ROOT)$(MANDIR)/man1b/test.1b

clean:
	rm -f test test.o test_sus test_sus.o main.o helper.o helper_sus.o version.o version_sus.o test_ucb test_ucb.o version_ucb.o core log *~

main.o: main.c defs.h
helper.o: helper.c defs.h
helper_sus.o: helper.c defs.h
test.o: test.c defs.h
test_sus.o: test.c defs.h
test_ucb.o: test.c defs.h
