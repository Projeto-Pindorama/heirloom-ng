#
# Sccsid @(#)heirloom.spec	1.33 (gritter) 01/27/07
#
Summary: Heirloom Toolchest: A collection of standard Unix utilities
Name: heirloom
Version: 070715
Release: 1
License: Other
Source: %{name}-%{version}.tar.bz2
Group: System Environment/Base
Vendor: Gunnar Ritter <gunnarr@acm.org>
URL: <http://heirloom.sourceforge.net>
BuildRoot: %{_tmppath}/%{name}-root
BuildRequires: heirloom-devtools

#
# The problem here is that package names differ between Linux distributions:
#
# * libz and libbz2 are in varying development packages.
# 
# For this reason, we use absolute path names and assume that all of this
# stuff is in /usr.
#
BuildRequires: /usr/lib/libz.so /usr/lib/libbz2.so
BuildRequires: /usr/include/zlib.h /usr/include/bzlib.h

#
# The following definitions can be changed if desired.
#
%define	usr		/usr
%define	defbin		%{usr}/5bin
%define	sv3bin		%{defbin}
%define	s42bin		%{defbin}/s42
%define	susbin		%{defbin}/posix
%define	su3bin		%{defbin}/posix2001
%define	ucbbin		%{usr}/ucb
%define	ccsbin		%{usr}/ccs/bin
%define	deflib		%{usr}/5lib
%define	defsbin		%{defbin}
%define	magic		%{deflib}/magic

%define	mandir		%{usr}/share/man/5man

%define	dfldir		/etc/default
%define	spellhist	/var/adm/spellhist
%define	sulog		/var/log/sulog

%define	xcc		gcc
%define	cflags2		'-O2 -fomit-frame-pointer'
%define	cflagss		'-Os -fomit-frame-pointer'
%define	cflagsu		'-O2 -fomit-frame-pointer -funroll-loops'
%define	cflags		'-O -fomit-frame-pointer'
%define	cppflags	'-D__NO_STRING_INLINES -D_GNU_SOURCE'
%define	yacc		/usr/ccs/bin/yacc
%define	lex		/usr/ccs/bin/lex

%define	lcurs		-lncurses
%define	libz		-lz
%define	use_zlib	1
%define	libbz2		-lbz2
%define	use_bzlib	1

%if %(test -x %{sv3bin}/sh && echo 1 || echo 0)
%define	shell		%{sv3bin}/sh
%else
%define	shell		/bin/sh
%endif

#
# Combine the settings defined above.
#
%define	p_flags	SHELL=%{shell} ROOT=%{buildroot} DEFBIN=%{defbin} SV3BIN=%{sv3bin} S42BIN=%{s42bin} SUSBIN=%{susbin} SU3BIN=%{su3bin} UCBBIN=%{ucbbin} CCSBIN=%{ccsbin} DEFLIB=%{deflib} DEFSBIN=%{defsbin} MANDIR=%{mandir} DFLDIR=%{dfldir} SPELLHIST=%{spellhist} SULOG=%{sulog} MAGIC=%{magic}
%define	c_flags	CC=%{xcc} CFLAGS2=%{cflags2} CFLAGSS=%{cflagss} CFLAGSU=%{cflagsu} CFLAGS=%{cflags} CPPFLAGS=%{cppflags} LCURS=%{lcurs} LIBZ=%{libz} USE_ZLIB=%{use_zlib} LIBBZ2=%{libbz2} USE_BZLIB=%{use_bzlib} TTYGRP= YACC=%{yacc} LEX=%{lex}
%define	makeflags %{p_flags} %{c_flags}

%description
The Heirloom Toolchest is a collection of standard Unix utilities. Highlights
are:
    * Derived from original Unix material released as open source by Caldera.
    * Up to four versions of each utility corresponding to SVID3/SVR4,
      SVID4/SVR4.2, POSIX.2/SUSV2, and 4BSD (SVR4 /usr/ucb).
    * Support for lines of arbitrary length and in many cases binary input data.
    * Support for multibyte character sets, especially UTF-8.
    * More than 100 individual utilities including bc, cpio, diff, ed, file,
      find, grep, man, nawk, oawk, pax, ps, sed, sort, spell, and tar.
    * The cpio and pax utilities can read and write zip files, GNU tar files,
      and the cpio formats of several commercial Unix systems.
    * Extensive documentation including a manual page for any utility.

%prep
rm -rf %{buildroot}
%setup

%build
make %{makeflags}

%install
make %{makeflags} install

#
# Retain the subdirectory structure of NOTES files for /usr/share/doc.
#
mkdir _doc
find . -name NOTES -depth | cpio -pdm _doc

#
# Generate the list of files; make sure that each file is listed only once
# and add attributes.
#
rm -f filelist.rpm
for f in %{defbin} %{sv3bin} %{s42bin} %{susbin} %{ucbbin} %{ccsbin} \
	%{deflib} %{defsbin} %{magic}
do
	if test -d %{buildroot}/$f
	then
		(cd %{buildroot}/$f; find * -type f -o -type l) | sed "s:^:$f/:"
	else
		echo $f
	fi
done | sort -u | sed '
	1i\
%defattr(-,root,root)\
%config(noreplace) %{dfldir}/*\
%config(noreplace) %{spellhist}\
%config(noreplace) %{sulog}\
%{mandir}\
%doc README\
%doc LICENSE\
%doc _doc/*

	/\/ps$/		s:^:%attr(4755,root,root) :
	/\/shl$/	s:^:%attr(2755,root,utmp) :
	/\/su$/		s:^:%attr(4755,root,root) :
' >filelist.rpm

%if "%{dfldir}" == "/etc/default"
%post
#
# On RedHat Linux, /etc/default is part of the default package shadow-utils
# and has a mode of 750. The only file below is /etc/default/useradd with a
# mode of 600. It thus seems safe to allow directory access for all users.
#
chmod a+x %{dfldir}
%endif

%clean
cd .. && rm -rf %{_builddir}/%{name}-%{version}
rm -rf %{buildroot}

%files -f filelist.rpm
