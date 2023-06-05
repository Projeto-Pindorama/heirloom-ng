# Heirloom NG

A collection of standard Unix utilities that is intended to provide maximum
compatibility with traditional Unix while incorporating additional features
necessary today.  
This fork aims to replace the old set of patches used by Copacabana Linux, by
implementing tools that were lacking and enhancing compatibility with newer
operating systems.

**Note**: Many of the text here still just verbatim copies from the original
README (at [the old README](./DOCUMENTATION.old/README)), so many things here
shall and will be updated.

## Building and installing

This fork made some modifications to the source code, mainly regarding
portability, so we cannot confirm if Heirloom NG will build and run in the same
conditions that the original Heirloom Toolchest did before.

System environments that Heirloom NG is known to work:  

| Operating system | Version | Standard C library and C compiler |  
|------------------|---------|---------------------------------------------------------------------------------------|  
| Linux            | 5.15.0  | musl C library (1.2.1 and above) and GNU C Library (2.31 and above). GNU Compiler Collection (version 10.2.1 and above) |  

System environments that the Heirloom Toolchest was known to work (and that we
cannot verify if it stills working):

| Operating system | Version                      | Standard C library and C compiler                                                    |  
|------------------|------------------------------|--------------------------------------------------------------------------------------|  
| Linux            | Kernel 2.4 and above         | GNU C Library 2.2 and above, diet libc, uClibc. GNU Compiler Collection and Intel C. |  
| Solaris          | 8 and above                  | Sun C and the GNU Compiler Collection.                                               |  
| Open UNIX        | 8.0.0                        | The native C compiler (cc) and the GNU Compiler Collection (via L.K.P.).             |  
| HP-UX            | B.11.11, B.11.22 and B.11.23 | HP-UX's ANSI C compiler and GNU Compiler Collection.                                 |  
| AIX              | 5.1                          | VisualAge C compiler and GNU Compiler Collection.                                    |  
| FreeBSD          | 4.9, 5.2.1 and 6.1           | Not informed in the original README.                                                 |  
| NetBSD           | 1.6, 2.0 and 3.0             | Not informed in the original README.                                                 |  
| OpenBSD          | 3.4 and 3.8                  | Not informed in the original README.                                                 |  
| DragonFlyBSD     | 1.3.7-DEVELOPMENT            | Not informed in the original README.                                                 |  
| Mac OS X         | 10.4.3 and above             | Not informed in the original README.                                                 |  


### Prerequisites 

The following prerequisites are necessary:

<dl>
<dt><tt>yacc</tt> and <tt>lex</tt></dt>
<dd>Several of the included <tt>yacc</tt> and <tt>lex</tt> programs need to access internals of the <tt>yacc</tt> and <tt>lex</tt> implementations beyond the respective POSIX specifications.<br>
This has been made to work with Unix <tt>yacc</tt> and <tt>lex</tt> as well as with many versions of Berkeley <tt>yacc</tt>, <tt>bison</tt>, and <tt>flex</tt>. However, recent changes in these programs may result in incompatibilities.<br>
If this happens, install the “Heirloom Development Tools” package first and use the <tt>yacc</tt> and <tt>lex</tt> versions provided with it. You could also fix the problem on your own; if you’ve found a bug and are willing to fix it, take a look at the “Chip in!” section.</dd>
</dl>

The following prerequisites are optional:

<dl>
<dt>Heirloom Bourne shell</dt>
<dd>It is recommended that the <a href="http://heirloom.sourceforge.net/sh.html">Heirloom Bourne shell</a> is installed first.<br>
Although the tools itself work with any shell, the original Bourne shell theorically ensures maximum compatibility with the traditional Unix behavior.<br>
In any case, a default POSIX Shell implementation is good to go.</dd>
<dt>Gzip</dt>
<dd>It is invoked by <code>cpio</code>/<code>pax</code>, <code>file</code> and <code>tar</code> when a Gzip-compressed file is encountered.<br>
In the modern World (also known as “nowadays”), computers can execute<br>
multiple jobs at the same time, so pigz would be preferred over the traditional Gzip.<br>
You can get pigz at <a href="https://zlib.net/pigz/">https://zlib.net/pigz/</a></dd>
<dt>bzip2</dt>
<dd>It is also invoked by <code>cpio</code>/<code>pax</code>, <code>file</code> and <code>tar</code>, but<br>
when a bzip2 compressed file is encountered.<br>
You can get bzip2 at <a href="https://sourceware.org/bzip2/">https://sourceware.org/bzip2/</a></dd>
<dt>libz</dt>
<dd>It is needed for enabling <code>cpio</code>/<code>pax</code> capacibilities to create deflate-compressed entries in zip files; reading of such entries works without it, through.<br>
There are many implementations of libz out there.<br>
Heirloom NG is currently tested being linked against Sortix libz<br>
on Copacabana Linux, and you can get it here: <a href="https://sortix.org/libz/">https://sortix.org/libz/</a></dd>
<dt>libbz2</dt>
<dd>It is needed enabling <code>cpio</code>/<code>pax</code> capacibilities to read or create bzip2-compressed entries in zip files.<br>
You can get it in the same package that comes bzip2.</dd>
</dl>

### Building Heirloom NG

Heirloom is built manually --- in other words, you will need to know how to
build a package manually using make, and also a little bit about the build
system being used.  
The first thing to understand is the build system. This is actually quite
simple: Every directory contains a file named ``Makefile.mk`` that includes
directory-specific make instructions. To generate the real ``Makefile``,
configuration settings are prepended to the directory-specific file.  
You have to edit these configuration settings before you start compiling;
they are located in the file ``build/mk.config`` which is also in make
syntax.  

Follow the descriptions in this file and select appropriate values for
your system. Read everything carefully and you will pretty much get it
in some minutes.

Be very careful at this step! If you overwrite your system utilities
or put the Heirloom Toolchest at an early place in your system's ``PATH``,
some of your shell scripts or of those of your vendor may cease to
work.  
This is because utilities from the Heirloom Toolchest may behave
differently than the ones supplied by your vendor. You may wish to
correct this later, but the first build is definitively not the right
time. - If you want to stay with the default ``/usr/5bin`` path on Solaris,
be sure to remove the existing symbolic link to ``/usr/bin``.

If you don't know what path names to select, you can ask us at the GitHub
Issues. Just keep in mind that, as per the licence terms, we do not offer
warrant for any problems caused by anyone.

After finishing the configuration, type ``make`` and '``make install``',
then use the tools found at the location you selected before.

## Chip in!

### Hacking the Heirloom Toolchest

One of the considerations the Heirloom Toolchest is designed along
is accessibility for other developers. Each utility was made largely
independent from other ones, to the extent of duplicate functionality
at some places. There are just three libraries you have to know about:

1. The 'common' library, found in ``libcommon/``, provides the
following functionality:

* Traditional ('simple') regular expressions, available as full
source to ``#include`` in the old-style ed-hack ``regexp.h`` and
as normal library functions in ``regexpr.h``. You should
definitively use the latter if you want to use simple REs in new
applications;
* Traditional file name matching; manually include the prototype
'``extern int gmatch(const char *, const char *);``' if you want
to use it;
* SVID4-style error reporting: ``#include <pfmt.h>``;
* Locale-independent character class functions in ``<asciitype.h>``.  
These are necessary e.g. to recognize programming language syntax
white space if ``LC_CTYPE`` is used otherwise (if you do not want to
handle no-break-space characters etc. as word delimiters);
* Functions for reading directory entries with a file descriptor
obtained from ``open()``, in ``<getdir.h>``. These are only useful if
you descend a directory hierarchy with fchdir() and have to care about
the limit of open file descriptors, as the ``file``(1) and ``rm``(1)
tools; otherwise, stay with the standard ``readdir()``;
* A ``getopt()`` that has the traditional functionality, and just the
traditional functionality. This is here mainly because GNU's ``getopt()``
is so annoyingly non-standard;
* Functions for buffered input and output in ``<iblok.h>`` and ``<oblok.h>``,
respectively. They can be used as a replacement for stdio in simple
cases; the main reason for their existence is the desire to read
lines of arbitrary length without the slow-down caused by ``getc()``.

2. A modified version of Caldera's 'UNIX(R) Regular Expression Library'
in the ``libuxre/`` directory.
This is used for (mostly) POSIX.2-compatible regular expression matching.
It provides a syntax without too much extension fuss, and good
performance, since it includes both a DFA and a NFA style matcher and
uses either of them depending on the expression and the need for
subexpression locations.

3. The widechar portability library in the ``libwchar/`` directory.
This is only used for system environments that do not provide appropriate
wide character functionality in their own libc. It currently just
does simple copying between bytes and wide characters and thus does
not handle anything than plain ASCII well. If other wide character
functions than those currently present are used in new code, it will
be necessary to add those function to libwchar as well.

To go on, you now only need to know about the sources in the directory
of the utility you want to hack. This should make it not too difficult,
even though Gunnar had generally stayed with good old Unix programming
style (few comments, short variable names - but not too much to read
either).

### Extending the Heirloom Toolchest

If you want to add an extension to a utility, just go on and do it - as
you've read in the section above, accessibility for hacking is intended
to be easy. For my version, though, the concept is to have not too many
extensions. I sometimes include extensions because the functionality is
really expected these days (such as ``diff -u``), adds a real advantage
for interactive use (such as colored output for ``ls``), or is widely
used for convenience and does not add much overhead (such as ``grep -r``).
I am not much interested in extensions that change the behavior of tools
for use in scripts, though. Such extensions lead to subtly incompatible
shell scripts, and these should be avoided by all means. It is much better
to make a fresh start by creating a new utility with a different name for
such cases. Such utilities are then beyond the scope of this collection.
In general, chances are rather low that I will accept your extension for
my version. But that doesn't mean that you should not do it for yourself.

### Porting the Heirloom Toolchest

If you want to port the Heirloom Toolchest to a new environment,
check that the following conditions are satisfied:

* The environment is a Unix-like system, e.g. the file system is
  case-sensitive, there is a unique ``dev_t`` and ``ino_t`` combination
  for each file i-node, there are at least POSIX.1 calls etc. The code
  heavily relies on this, and I'm not interested in changes for
  non-Unix environments for a variety of reasons;
* The compiler supports '``long long``' and distinguished 8-bit, 16-bit,
  32-bit, and 64-bit data types. The system stdio library supports
  ``printf("%lld")``.
* The compiler does not insert padding between two structure fields of
  type '``char``' or array of '``char``'.
* You know how to read directory contents without ``opendir()`` and friends,
  using a file descriptor obtained from ``open()``. On most systems, this
  involves either the System V function ``getdents()`` or the BSD function
  getdirentries(). It may also be possible to use ``read()``, although this
  should only be the last resort. Such functionality may or may not be
  documented; search for it in ``/usr/include`` if in doubt. If ``getdents()``
  or ``getdirentries()`` exist, you can probably adjust the existing code
  in '``libcommon/getdir.c``'.
* You know how to obtain a process listing in C.
  This may be as easy as reading ``/proc`` (you can probably re-use existing code then),
  but might also require that you know how to read structures from ``/dev/kmem`` in the
  worst case. Documentation for this is usually sparse; look around in ``/usr/include``
  or search the web for existing open source code that performs such lookups (e.g.
  '``lsof``', '``libgtop``').
* You know how to code tape device control (``<sys/mtio.h>`` or the like).
  This is usually documented, so if in doubt, just read the manual
  pages and header files.

If all of these are true, you can expect the work to be done within
a day or so.

The Heirloom Toolchest is coded with The Open Group's 'Single Unix
Specification', Version 3, 2001, Base Definitions and Headers, and
System Interfaces, in mind. If the system is Unix-like, missing
functionality can usually be emulated. For a number of functions,
such as those for wide-character support, utmpx access, simplified
signal management (``sigset()`` etc.), emulation code is already
present in the libcommon and libwchar directories; enable it if
necessary.

### Reporting bugs

Before you report behavior that you suspect to be a bug, check at least
the following to be sure not to report a feature:

* Does it leave a core dump? If yes, it's certainly a bug (unless you
  sent a signal, of course);
* Read the manual page and the source for the utility carefully. Did
  you use the utility properly?
* If the utility is used within a shell script or with another user's
  environment, try how it behaves if used standalone;
* If the behavior disturbs you just because your vendor's utilities do
  something different, that alone is no reason to report it. The Heirloom
  Toolchest is oriented on System V behavior. Verify that real System V
  derivatives behave differently before reporting;
* If you are using one of the POSIX personalities and the specification
  says something different, it is likely to be a bug. But be careful to
  read the respective specification itself; some incompatibilities to
  the wording of the specification may even exist in certified systems,
  so you cannot rely on a vendor's implementation or documentation here.

If you went through these statements and you're still sure that it is
a bug, report it.

## Authors

Gunnar Ritter <gunnarr@acm.org>  
Luiz Antônio  
Arthur Bacci
