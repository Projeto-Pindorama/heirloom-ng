all: priocntl priocntl_s42

priocntl: priocntl.o
	$(LD) $(LDFLAGS) priocntl.o $(LCOMMON) $(LWCHAR) $(LIBS) -o priocntl

priocntl.o: priocntl.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(IWCHAR) $(ICOMMON) -c priocntl.c

priocntl_s42: priocntl_s42.o
	$(LD) $(LDFLAGS) priocntl_s42.o $(LCOMMON) $(LWCHAR) $(LIBS) -o priocntl_s42

priocntl_s42.o: priocntl.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(IWCHAR) $(ICOMMON) -DS42 -c priocntl.c -o priocntl_s42.o

install: all
	uname=`uname`; \
	if test "$$uname" = Linux || test "$$uname" = FreeBSD; \
	then \
		$(UCBINST) -c priocntl $(ROOT)$(SV3BIN)/priocntl &&\
		$(STRIP) $(ROOT)$(SV3BIN)/priocntl &&\
		$(UCBINST) -c priocntl_s42 $(ROOT)$(S42BIN)/priocntl &&\
		$(STRIP) $(ROOT)$(S42BIN)/priocntl &&\
		$(MANINST) -c -m 644 priocntl.1 $(ROOT)$(MANDIR)/man1/priocntl.1 ;\
	else \
		exit 0; \
	fi

clean:
	rm -f priocntl priocntl.o priocntl_s42 priocntl_s42.o core log *~
