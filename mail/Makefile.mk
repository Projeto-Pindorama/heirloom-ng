OBJ = Dout.o Tout.o add_recip.o cat.o ckdlivopts.o cksaved.o \
	clr_hinfo.o copyback.o copylet.o copymt.o createmf.o del_recipl.o \
	delete.o done.o doopen.o dumpaff.o dumprcv.o errmsg.o gendeliv.o \
	getarg.o getcomment.o gethead.o goback.o init.o isheader.o isit.o \
	islocal.o istext.o legal.o lock.o maid.o main.o mkdead.o mta_ercode.o \
	new_recipl.o parse.o pckaffspot.o pckrcvspot.o pickFrom.o pipletr.o \
	poplist.o printhdr.o printmail.o pushlist.o savehdrs.o sel_disp.o \
	sendlist.o sendmail.o setsig.o stamp.o \
	casncmp.o copystream.o delempty.o getdomain.o maillock.o \
	popenvp.o skipspace.o strmove.o substr.o trimnl.o xgetenv.o \
	fgetline.o

all: mail

mail: $(OBJ)
	$(LD) $(LDFLAGS) $(OBJ) $(LCOMMON) $(LWCHAR) $(LIBS) $(LSOCKET) -o mail

.c.o: ; $(CC) -c $(CFLAGS) $(CPPFLAGS) $(GNUFL) $(IWCHAR) $(ICOMMON) -DSHELL='"$(SHELL)"' -c $<

install: all
	$(UCBINST) -c mail $(ROOT)$(DEFBIN)/mail
	$(STRIP) $(ROOT)$(DEFBIN)/mail
	$(MANINST) -c -m 644 mail.1 $(ROOT)$(MANDIR)/man1/mail.1

clean:
	rm -f mail $(OBJ) core log *~

Dout.o: Dout.c mail.h libmail.h maillock.h s_string.h
Tout.o: Tout.c mail.h libmail.h maillock.h s_string.h
add_recip.o: add_recip.c mail.h libmail.h maillock.h s_string.h
casncmp.o: casncmp.c libmail.h maillock.h s_string.h
cat.o: cat.c mail.h libmail.h maillock.h s_string.h
ckdlivopts.o: ckdlivopts.c mail.h libmail.h maillock.h s_string.h
cksaved.o: cksaved.c mail.h libmail.h maillock.h s_string.h
clr_hinfo.o: clr_hinfo.c mail.h libmail.h maillock.h s_string.h
copyback.o: copyback.c mail.h libmail.h maillock.h s_string.h
copylet.o: copylet.c mail.h libmail.h maillock.h s_string.h
copymt.o: copymt.c mail.h libmail.h maillock.h s_string.h
copystream.o: copystream.c libmail.h maillock.h s_string.h
createmf.o: createmf.c mail.h libmail.h maillock.h s_string.h
del_recipl.o: del_recipl.c mail.h libmail.h maillock.h s_string.h
delempty.o: delempty.c libmail.h maillock.h s_string.h
delete.o: delete.c mail.h libmail.h maillock.h s_string.h
done.o: done.c mail.h libmail.h maillock.h s_string.h
doopen.o: doopen.c mail.h libmail.h maillock.h s_string.h
dumpaff.o: dumpaff.c mail.h libmail.h maillock.h s_string.h
dumprcv.o: dumprcv.c mail.h libmail.h maillock.h s_string.h
errmsg.o: errmsg.c mail.h libmail.h maillock.h s_string.h
gendeliv.o: gendeliv.c mail.h libmail.h maillock.h s_string.h
getarg.o: getarg.c mail.h libmail.h maillock.h s_string.h
getcomment.o: getcomment.c mail.h libmail.h maillock.h s_string.h
getdomain.o: getdomain.c libmail.h maillock.h s_string.h
gethead.o: gethead.c mail.h libmail.h maillock.h s_string.h
goback.o: goback.c mail.h libmail.h maillock.h s_string.h
init.o: init.c mail.h libmail.h maillock.h s_string.h
isheader.o: isheader.c mail.h libmail.h maillock.h s_string.h
isit.o: isit.c mail.h libmail.h maillock.h s_string.h
islocal.o: islocal.c mail.h libmail.h maillock.h s_string.h
istext.o: istext.c mail.h libmail.h maillock.h s_string.h
legal.o: legal.c mail.h libmail.h maillock.h s_string.h
lock.o: lock.c mail.h libmail.h maillock.h s_string.h
maid.o: maid.c
maillock.o: maillock.c maillock.h
main.o: main.c mail.h libmail.h maillock.h s_string.h
mkdead.o: mkdead.c mail.h libmail.h maillock.h s_string.h
mta_ercode.o: mta_ercode.c mail.h libmail.h maillock.h s_string.h
new_recipl.o: new_recipl.c mail.h libmail.h maillock.h s_string.h
parse.o: parse.c mail.h libmail.h maillock.h s_string.h
pckaffspot.o: pckaffspot.c mail.h libmail.h maillock.h s_string.h
pckrcvspot.o: pckrcvspot.c mail.h libmail.h maillock.h s_string.h
pickFrom.o: pickFrom.c mail.h libmail.h maillock.h s_string.h
pipletr.o: pipletr.c mail.h libmail.h maillock.h s_string.h
popenvp.o: popenvp.c libmail.h maillock.h s_string.h
poplist.o: poplist.c mail.h libmail.h maillock.h s_string.h
printhdr.o: printhdr.c mail.h libmail.h maillock.h s_string.h
printmail.o: printmail.c mail.h libmail.h maillock.h s_string.h
pushlist.o: pushlist.c mail.h libmail.h maillock.h s_string.h
savehdrs.o: savehdrs.c mail.h libmail.h maillock.h s_string.h
sel_disp.o: sel_disp.c mail.h libmail.h maillock.h s_string.h
sendlist.o: sendlist.c mail.h libmail.h maillock.h s_string.h
sendmail.o: sendmail.c mail.h libmail.h maillock.h s_string.h
setsig.o: setsig.c mail.h libmail.h maillock.h s_string.h
skipspace.o: skipspace.c libmail.h maillock.h s_string.h
stamp.o: stamp.c mail.h libmail.h maillock.h s_string.h
strmove.o: strmove.c libmail.h maillock.h s_string.h
substr.o: substr.c libmail.h maillock.h s_string.h
trimnl.o: trimnl.c libmail.h maillock.h s_string.h
xgetenv.o: xgetenv.c libmail.h maillock.h s_string.h
