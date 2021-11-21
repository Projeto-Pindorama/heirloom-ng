all: libwchar.a

OBJ = mbtowc.o wctomb.o wctype.o mbstowcs.o wcwidth.o wcslen.o wcsncmp.o \
	mbrtowc.o wctfunc.o mblen.o
libwchar.a: fake $(OBJ)
	$(AR) -rv $@ $(OBJ)
	$(RANLIB) $@

fake:
	if test "x$(LWCHAR)" = x; \
	then \
		touch $(OBJ); \
		ar r libwchar.a $(OBJ); \
	fi

install:

clean:
	rm -f $(OBJ) core log *~

mbtowc.o: mbtowc.c wchar.h
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) -I. -c mbtowc.c

mblen.o: mblen.c wchar.h
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) -I. -c mblen.c

mbrtowc.o: mbrtowc.c wchar.h
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(ICOMMON) -I. -c mbrtowc.c

wctomb.o: wctomb.c wchar.h
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(ICOMMON) -I. -c wctomb.c

mbstowcs.o: mbstowcs.c wchar.h
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(ICOMMON) -I. -c mbstowcs.c

wcwidth.o: wcwidth.c wchar.h
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(ICOMMON) -I. -c wcwidth.c

wcslen.o: wcslen.c wchar.h
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(ICOMMON) -I. -c wcslen.c

wcsncmp.o: wcsncmp.c wchar.h
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(ICOMMON) -I. -c wcsncmp.c

wctype.o: wctype.c wchar.h
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(ICOMMON) -I. -c wctype.c

wctfunc.o: wctfunc.c wchar.h
	$(CC) $(CFLAGSS) $(CPPFLAGS) $(LARGEF) $(ICOMMON) -I. -c wctfunc.c

MRPROPER = libwchar.a
