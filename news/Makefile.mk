all: news

news: news.o
	$(LD) $(LDFLAGS) news.o $(LCOMMON) $(LWCHAR) $(LIBS) -o news

news.o: news.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(XO5FL) $(IWCHAR) $(ICOMMON) -c news.c

install: all
	$(UCBINST) -c news $(ROOT)$(DEFBIN)/news
	$(STRIP) $(ROOT)$(DEFBIN)/news
	$(MANINST) -c -m 644 news.1 $(ROOT)$(MANDIR)/man1/news.1

clean:
	rm -f news news.o core log *~
