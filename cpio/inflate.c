/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, May 2003.
 *
 * Derived from Info-ZIP 5.50.
 *
 * Sccsid @(#)inflate.c	1.6 (gritter) 10/13/04
 */
/*
This is version 2002-Feb-16 of the Info-ZIP copyright and license.
The definitive version of this document should be available at
ftp://ftp.info-zip.org/pub/infozip/license.html indefinitely.


Copyright (c) 1990-2002 Info-ZIP.  All rights reserved.

For the purposes of this copyright and license, "Info-ZIP" is defined as
the following set of individuals:

   Mark Adler, John Bush, Karl Davis, Harald Denker, Jean-Michel Dubois,
   Jean-loup Gailly, Hunter Goatley, Ian Gorman, Chris Herborth, Dirk Haase,
   Greg Hartwig, Robert Heath, Jonathan Hudson, Paul Kienitz, David Kirschbaum,
   Johnny Lee, Onno van der Linden, Igor Mandrichenko, Steve P. Miller,
   Sergio Monesi, Keith Owens, George Petrov, Greg Roelofs, Kai Uwe Rommel,
   Steve Salisbury, Dave Smith, Christian Spieler, Antoine Verheijen,
   Paul von Behren, Rich Wales, Mike White

This software is provided "as is," without warranty of any kind, express
or implied.  In no event shall Info-ZIP or its contributors be held liable
for any direct, indirect, incidental, special or consequential damages
arising out of the use of or inability to use this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. Redistributions of source code must retain the above copyright notice,
       definition, disclaimer, and this list of conditions.

    2. Redistributions in binary form (compiled executables) must reproduce
       the above copyright notice, definition, disclaimer, and this list of
       conditions in documentation and/or other materials provided with the
       distribution.  The sole exception to this condition is redistribution
       of a standard UnZipSFX binary as part of a self-extracting archive;
       that is permitted without inclusion of this license, as long as the
       normal UnZipSFX banner has not been removed from the binary or disabled.

    3. Altered versions--including, but not limited to, ports to new operating
       systems, existing ports with new graphical interfaces, and dynamic,
       shared, or static library versions--must be plainly marked as such
       and must not be misrepresented as being the original source.  Such
       altered versions also must not be misrepresented as being Info-ZIP
       releases--including, but not limited to, labeling of the altered
       versions with the names "Info-ZIP" (or any variation thereof, including,
       but not limited to, different capitalizations), "Pocket UnZip," "WiZ"
       or "MacZip" without the explicit permission of Info-ZIP.  Such altered
       versions are further prohibited from misrepresentative use of the
       Zip-Bugs or Info-ZIP e-mail addresses or of the Info-ZIP URL(s).

    4. Info-ZIP retains the right to use the names "Info-ZIP," "Zip," "UnZip,"
       "UnZipSFX," "WiZ," "Pocket UnZip," "Pocket Zip," and "MacZip" for its
       own source and binary releases.
*/
/*
  Copyright (c) 1990-2002 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2000-Apr-09 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/* inflate.c -- by Mark Adler
   version c17a, 04 Feb 2001 */


/* Copyright history:
   - Starting with UnZip 5.41 of 16-April-2000, this source file
     is covered by the Info-Zip LICENSE cited above.
   - Prior versions of this source file, found in UnZip source packages
     up to UnZip 5.40, were put in the public domain.
     The original copyright note by Mark Adler was:
         "You can do whatever you like with this source file,
         though I would prefer that if you modify it and
         redistribute it that you include comments to that effect
         with your name and the date.  Thank you."

   History:
   vers    date          who           what
   ----  ---------  --------------  ------------------------------------
    a    ~~ Feb 92  M. Adler        used full (large, one-step) lookup table
    b1   21 Mar 92  M. Adler        first version with partial lookup tables
    b2   21 Mar 92  M. Adler        fixed bug in fixed-code blocks
    b3   22 Mar 92  M. Adler        sped up match copies, cleaned up some
    b4   25 Mar 92  M. Adler        added prototypes; removed window[] (now
                                    is the responsibility of unzip.h--also
                                    changed name to slide[]), so needs diffs
                                    for unzip.c and unzip.h (this allows
                                    compiling in the small model on MSDOS);
                                    fixed cast of q in huft_build();
    b5   26 Mar 92  M. Adler        got rid of unintended macro recursion.
    b6   27 Mar 92  M. Adler        got rid of nextbyte() routine.  fixed
                                    bug in inflate_fixed().
    c1   30 Mar 92  M. Adler        removed lbits, dbits environment variables.
                                    changed BMAX to 16 for explode.  Removed
                                    OUTB usage, and replaced it with flush()--
                                    this was a 20% speed improvement!  Added
                                    an explode.c (to replace unimplod.c) that
                                    uses the huft routines here.  Removed
                                    register union.
    c2    4 Apr 92  M. Adler        fixed bug for file sizes a multiple of 32k.
    c3   10 Apr 92  M. Adler        reduced memory of code tables made by
                                    huft_build significantly (factor of two to
                                    three).
    c4   15 Apr 92  M. Adler        added NOMEMCPY do kill use of memcpy().
                                    worked around a Turbo C optimization bug.
    c5   21 Apr 92  M. Adler        added the WSIZE #define to allow reducing
                                    the 32K window size for specialized
                                    applications.
    c6   31 May 92  M. Adler        added some typecasts to eliminate warnings
    c7   27 Jun 92  G. Roelofs      added some more typecasts (444:  MSC bug).
    c8    5 Oct 92  J-l. Gailly     added ifdef'd code to deal with PKZIP bug.
    c9    9 Oct 92  M. Adler        removed a memory error message (~line 416).
    c10  17 Oct 92  G. Roelofs      changed ULONG/UWORD/byte to ulg/ush/uch,
                                    removed old inflate, renamed inflate_entry
                                    to inflate, added Mark's fix to a comment.
   c10.5 14 Dec 92  M. Adler        fix up error messages for incomplete trees.
    c11   2 Jan 93  M. Adler        fixed bug in detection of incomplete
                                    tables, and removed assumption that EOB is
                                    the longest code (bad assumption).
    c12   3 Jan 93  M. Adler        make tables for fixed blocks only once.
    c13   5 Jan 93  M. Adler        allow all zero length codes (pkzip 2.04c
                                    outputs one zero length code for an empty
                                    distance tree).
    c14  12 Mar 93  M. Adler        made inflate.c standalone with the
                                    introduction of inflate.h.
   c14b  16 Jul 93  G. Roelofs      added (unsigned) typecast to w at 470.
   c14c  19 Jul 93  J. Bush         changed v[N_MAX], l[288], ll[28x+3x] arrays
                                    to static for Amiga.
   c14d  13 Aug 93  J-l. Gailly     de-complicatified Mark's c[*p++]++ thing.
   c14e   8 Oct 93  G. Roelofs      changed memset() to memzero().
   c14f  22 Oct 93  G. Roelofs      renamed quietflg to qflag; made Trace()
                                    conditional; added inflate_free().
   c14g  28 Oct 93  G. Roelofs      changed l/(lx+1) macro to pointer (Cray bug)
   c14h   7 Dec 93  C. Ghisler      huft_build() optimizations.
   c14i   9 Jan 94  A. Verheijen    set fixed_t{d,l} to NULL after freeing;
                    G. Roelofs      check NEXTBYTE macro for EOF.
   c14j  23 Jan 94  G. Roelofs      removed Ghisler "optimizations"; ifdef'd
                                    EOF check.
   c14k  27 Feb 94  G. Roelofs      added some typecasts to avoid warnings.
   c14l   9 Apr 94  G. Roelofs      fixed split comments on preprocessor lines
                                    to avoid bug in Encore compiler.
   c14m   7 Jul 94  P. Kienitz      modified to allow assembler version of
                                    inflate_codes() (define ASM_INFLATECODES)
   c14n  22 Jul 94  G. Roelofs      changed fprintf to macro for DLL versions
   c14o  23 Aug 94  C. Spieler      added a newline to a debug statement;
                    G. Roelofs      added another typecast to avoid MSC warning
   c14p   4 Oct 94  G. Roelofs      added (voidp *) cast to free() argument
   c14q  30 Oct 94  G. Roelofs      changed fprintf macro to MESSAGE()
   c14r   1 Nov 94  G. Roelofs      fixed possible redefinition of CHECK_EOF
   c14s   7 May 95  S. Maxwell      OS/2 DLL globals stuff incorporated;
                    P. Kienitz      "fixed" ASM_INFLATECODES macro/prototype
   c14t  18 Aug 95  G. Roelofs      added UZinflate() to use zlib functions;
                                    changed voidp to zvoid; moved huft_build()
                                    and huft_free() to end of file
   c14u   1 Oct 95  G. Roelofs      moved G into definition of MESSAGE macro
   c14v   8 Nov 95  P. Kienitz      changed ASM_INFLATECODES to use a regular
                                    call with __G__ instead of a macro
    c15   3 Aug 96  M. Adler        fixed bomb-bug on random input data (Adobe)
   c15b  24 Aug 96  M. Adler        more fixes for random input data
   c15c  28 Mar 97  G. Roelofs      changed USE_ZLIB fatal exit code from
                                    PK_MEM2 to PK_MEM3
    c16  20 Apr 97  J. Altman       added memzero(v[]) in huft_build()
   c16b  29 Mar 98  C. Spieler      modified DLL code for slide redirection
   c16c  04 Apr 99  C. Spieler      fixed memory leaks when processing gets
                                    stopped because of input data errors
   c16d  05 Jul 99  C. Spieler      take care of FLUSH() return values and
                                    stop processing in case of errors
    c17  31 Dec 00  C. Spieler      added preliminary support for Deflate64
   c17a  04 Feb 01  C. Spieler      complete integration of Deflate64 support
   c17b  16 Feb 02  C. Spieler      changed type of "extra bits" arrays and
                                    corresponding huft_buid() parameter e from
                                    ush into uch, to save space
 */


/*
   Inflate deflated (PKZIP's method 8 compressed) data.  The compression
   method searches for as much of the current string of bytes (up to a
   length of 258) in the previous 32K bytes.  If it doesn't find any
   matches (of at least length 3), it codes the next byte.  Otherwise, it
   codes the length of the matched string and its distance backwards from
   the current position.  There is a single Huffman code that codes both
   single bytes (called "literals") and match lengths.  A second Huffman
   code codes the distance information, which follows a length code.  Each
   length or distance code actually represents a base value and a number
   of "extra" (sometimes zero) bits to get to add to the base value.  At
   the end of each deflated block is a special end-of-block (EOB) literal/
   length code.  The decoding process is basically: get a literal/length
   code; if EOB then done; if a literal, emit the decoded byte; if a
   length then get the distance and emit the referred-to bytes from the
   sliding window of previously emitted data.

   There are (currently) three kinds of inflate blocks: stored, fixed, and
   dynamic.  The compressor outputs a chunk of data at a time and decides
   which method to use on a chunk-by-chunk basis.  A chunk might typically
   be 32K to 64K, uncompressed.  If the chunk is uncompressible, then the
   "stored" method is used.  In this case, the bytes are simply stored as
   is, eight bits per byte, with none of the above coding.  The bytes are
   preceded by a count, since there is no longer an EOB code.

   If the data are compressible, then either the fixed or dynamic methods
   are used.  In the dynamic method, the compressed data are preceded by
   an encoding of the literal/length and distance Huffman codes that are
   to be used to decode this block.  The representation is itself Huffman
   coded, and so is preceded by a description of that code.  These code
   descriptions take up a little space, and so for small blocks, there is
   a predefined set of codes, called the fixed codes.  The fixed method is
   used if the block ends up smaller that way (usually for quite small
   chunks); otherwise the dynamic method is used.  In the latter case, the
   codes are customized to the probabilities in the current block and so
   can code it much better than the pre-determined fixed codes can.

   The Huffman codes themselves are decoded using a multi-level table
   lookup, in order to maximize the speed of decoding plus the speed of
   building the decoding tables.  See the comments below that precede the
   lbits and dbits tuning parameters.

   GRR:  return values(?)
           0  OK
           1  incomplete table
           2  bad input
           3  not enough memory
         the following return codes are passed through from FLUSH() errors
           50 (PK_DISK)   "overflow of output space"
           80 (IZ_CTRLC)  "canceled by user's request"
 */


/*
   Notes beyond the 1.93a appnote.txt:

   1. Distance pointers never point before the beginning of the output
      stream.
   2. Distance pointers can point back across blocks, up to 32k away.
   3. There is an implied maximum of 7 bits for the bit length table and
      15 bits for the actual data.
   4. If only one code exists, then it is encoded using one bit.  (Zero
      would be more efficient, but perhaps a little confusing.)  If two
      codes exist, they are coded using one bit each (0 and 1).
   5. There is no way of sending zero distance codes--a dummy must be
      sent if there are none.  (History: a pre 2.0 version of PKZIP would
      store blocks with no distance codes, but this was discovered to be
      too harsh a criterion.)  Valid only for 1.93a.  2.04c does allow
      zero distance codes, which is sent as one code of zero bits in
      length.
   6. There are up to 286 literal/length codes.  Code 256 represents the
      end-of-block.  Note however that the static length tree defines
      288 codes just to fill out the Huffman codes.  Codes 286 and 287
      cannot be used though, since there is no length base or extra bits
      defined for them.  Similarily, there are up to 30 distance codes.
      However, static trees define 32 codes (all 5 bits) to fill out the
      Huffman codes, but the last two had better not show up in the data.
   7. Unzip can check dynamic Huffman blocks for complete code sets.
      The exception is that a single code would not be complete (see #4).
   8. The five bits following the block type is really the number of
      literal codes sent minus 257.
   9. Length codes 8,16,16 are interpreted as 13 length codes of 8 bits
      (1+6+6).  Therefore, to output three times the length, you output
      three codes (1+1+1), whereas to output four times the same length,
      you only need two codes (1+3).  Hmm.
  10. In the tree reconstruction algorithm, Code = Code + Increment
      only if BitLength(i) is not zero.  (Pretty obvious.)
  11. Correction: 4 Bits: # of Bit Length codes - 4     (4 - 19)
  12. Note: length code 284 can represent 227-258, but length code 285
      really is 258.  The last length deserves its own, short code
      since it gets used a lot in very redundant files.  The length
      258 is special since 258 - 3 (the min match length) is 255.
  13. The literal/length and distance code bit lengths are read as a
      single stream of lengths.  It is possible (and advantageous) for
      a repeat code (16, 17, or 18) to go across the boundary between
      the two sets of lengths.
  14. The Deflate64 (PKZIP method 9) variant of the compression algorithm
      differs from "classic" deflate in the following 3 aspect:
      a) The size of the sliding history window is expanded to 64 kByte.
      b) The previously unused distance codes #30 and #31 code distances
         from 32769 to 49152 and 49153 to 65536.  Both codes take 14 bits
         of extra data to determine the exact position in their 16 kByte
         range.
      c) The last lit/length code #285 gets a different meaning. Instead
         of coding a fixed maximum match length of 258, it is used as a
         "generic" match length code, capable of coding any length from
         3 (min match length + 0) to 65538 (min match length + 65535).
         This means that the length code #285 takes 16 bits (!) of uncoded
         extra data, added to a fixed min length of 3.
      Changes a) and b) would have been transparent for valid deflated
      data, but change c) requires to switch decoder configurations between
      Deflate and Deflate64 modes.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cpio.h"
#include "unzip.h"

/*
    inflate.h must supply the uch slide[WSIZE] array, the zvoid typedef
    (void if (void *) is accepted, else char) and the NEXTBYTE,
    FLUSH() and memzero macros.  If the window size is not 32K, it
    should also define WSIZE.  If INFMOD is defined, it can include
    compiled functions to support the NEXTBYTE and/or FLUSH() macros.
    There are defaults for NEXTBYTE and FLUSH() below for use as
    examples of what those functions need to do.  Normally, you would
    also want FLUSH() to compute a crc on the data.  inflate.h also
    needs to provide these typedefs:

        typedef unsigned char uch;
        typedef unsigned short ush;
        typedef unsigned long ulg;

    This module uses the external functions malloc() and free() (and
    probably memset() or bzero() in the memzero() macro).  Their
    prototypes are normally found in <string.h> and <stdlib.h>.
 */

/* marker for "unused" huft code, and corresponding check macro */
#define INVALID_CODE 99
#define IS_INVALID_CODE(c)  ((c) == INVALID_CODE)

static int	inflate_codes(struct globals *Gp,
			struct huft *tl, struct huft *td,
                      int bl, int bd);
static int	inflate_stored(struct globals *Gp);
static int	inflate_fixed(struct globals *Gp);
static int	inflate_dynamic(struct globals *Gp);
static int	inflate_block(struct globals *Gp, int *e);

#define	FLUSH(n)	(flush(&G, redirSlide, (n)), 0)

/* The inflate algorithm uses a sliding 32K byte window on the uncompressed
   stream to find repeated byte strings.  This is implemented here as a
   circular buffer.  The index is updated simply by incrementing and then
   and'ing with 0x7fff (32K-1). */
/* It is left to other modules to supply the 32K area.  It is assumed
   to be usable as if it were declared "uch slide[32768];" or as just
   "uch *slide;" and then malloc'ed in the latter case.  The definition
   must be in unzip.h, included above. */


/* Tables for deflate from PKZIP's appnote.txt. */
/* - Order of the bit length code lengths */
static const unsigned border[] = {
        16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

/* - Copy lengths for literal codes 257..285 */
static const uint16_t cplens64[] = {
        3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
        35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 3, 0, 0};
        /* For Deflate64, the code 285 is defined differently. */
static const uint16_t cplens32[] = {
        3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
        35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0};
        /* note: see note #13 above about the 258 in this list. */
/* - Extra bits for literal codes 257..285 */
static const uint8_t cplext64[] = {
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
        3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 16, INVALID_CODE, INVALID_CODE};
static const uint8_t cplext32[] = {
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
        3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, INVALID_CODE, INVALID_CODE};

/* - Copy offsets for distance codes 0..29 (0..31 for Deflate64) */
static const uint16_t cpdist[] = {
        1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
        257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
        8193, 12289, 16385, 24577, 32769, 49153};

/* - Extra bits for distance codes 0..29 (0..31 for Deflate64) */
static const uint8_t cpdext64[] = {
        0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
        7, 7, 8, 8, 9, 9, 10, 10, 11, 11,
        12, 12, 13, 13, 14, 14};
static const uint8_t cpdext32[] = {
        0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
        7, 7, 8, 8, 9, 9, 10, 10, 11, 11,
        12, 12, 13, 13, INVALID_CODE, INVALID_CODE};

#  define MAXLITLENS 288
#  define MAXDISTS 32

/* Macros for inflate() bit peeking and grabbing.
   The usage is:

        NEEDBITS(j)
        x = b & mask_bits[j];
        DUMPBITS(j)

   where NEEDBITS makes sure that b has at least j bits in it, and
   DUMPBITS removes the bits from b.  The macros use the variable k
   for the number of bits in b.  Normally, b and k are register
   variables for speed and are initialized at the begining of a
   routine that uses these macros from a global bit buffer and count.

   In order to not ask for more bits than there are in the compressed
   stream, the Huffman tables are constructed to only ask for just
   enough bits to make up the end-of-block code (value 256).  Then no
   bytes need to be "returned" to the buffer at the end of the last
   block.  See the huft_build() routine.
 */

#  define NEEDBITS(n) {while(k<(n)){int c=NEXTBYTE;\
    if(c==EOF){retval=1;goto cleanup_and_exit;}\
    b|=((uint32_t)c)<<k;k+=8;}}

#define DUMPBITS(n) {b>>=(n);k-=(n);}

#define	Bits	32
#define	Nob	32
#define	Eob	31

/*
   Huffman code decoding is performed using a multi-level table lookup.
   The fastest way to decode is to simply build a lookup table whose
   size is determined by the longest code.  However, the time it takes
   to build this table can also be a factor if the data being decoded
   are not very long.  The most common codes are necessarily the
   shortest codes, so those codes dominate the decoding time, and hence
   the speed.  The idea is you can have a shorter table that decodes the
   shorter, more probable codes, and then point to subsidiary tables for
   the longer codes.  The time it costs to decode the longer codes is
   then traded against the time it takes to make longer tables.

   This results of this trade are in the variables lbits and dbits
   below.  lbits is the number of bits the first level table for literal/
   length codes can decode in one step, and dbits is the same thing for
   the distance codes.  Subsequent tables are also less than or equal to
   those sizes.  These values may be adjusted either when all of the
   codes are shorter than that, in which case the longest code length in
   bits is used, or when the shortest code is *longer* than the requested
   table size, in which case the length of the shortest code in bits is
   used.

   There are two different values for the two tables, since they code a
   different number of possibilities each.  The literal/length table
   codes 286 possible values, or in a flat code, a little over eight
   bits.  The distance table codes 30 possible values, or a little less
   than five bits, flat.  The optimum values for speed end up being
   about one bit more than those, so lbits is 8+1 and dbits is 5+1.
   The optimum values may differ though from machine to machine, and
   possibly even between compilers.  Your mileage may vary.
 */


static const int lbits = 9;    /* bits in base literal/length lookup table */
static const int dbits = 6;    /* bits in base distance lookup table */

#define	G	(*Gp)

static int
inflate_codes(struct globals *Gp,
		struct huft *tl, struct huft *td, int bl, int bd)
/*struct huft *tl, *td;*/ /* literal/length and distance decoder tables */
/*int bl, bd;*/           /* number of bits decoded by tl[] and td[] */
/* inflate (decompress) the codes in a deflated (compressed) block.
   Return an error code or zero if it all goes ok. */
{
  register unsigned e;  /* table entry flag/number of extra bits */
  unsigned d;           /* index for copy */
  uint32_t n;           /* length for copy (deflate64: might be 64k+2) */
  uint32_t w;           /* current window position (deflate64: up to 64k) */
  struct huft *t;       /* pointer to table entry */
  unsigned ml, md;      /* masks for bl and bd bits */
  register uint32_t b;       /* bit buffer */
  register unsigned k;  /* number of bits in bit buffer */
  int retval = 0;       /* error code returned: initialized to "no error" */


  /* make local copies of globals */
  b = G.bb;                       /* initialize bit buffer */
  k = G.bk;
  w = G.wp;                       /* initialize window position */


  /* inflate the coded data */
  ml = mask_bits[bl];           /* precompute masks for speed */
  md = mask_bits[bd];
  while (1)                     /* do until end of block */
  {
    NEEDBITS((unsigned)bl)
    t = tl + ((unsigned)b & ml);
    while (1) {
      DUMPBITS(t->b)

      if ((e = t->e) == 32)     /* then it's a literal */
      {
        redirSlide[w++] = (uint8_t)t->v.n;
        if (w == WSIZE)
        {
          if ((retval = FLUSH(w)) != 0) goto cleanup_and_exit;
          w = 0;
        }
        break;
      }

      if (e < 31)               /* then it's a length */
      {
        /* get length of block to copy */
        NEEDBITS(e)
        n = t->v.n + ((unsigned)b & mask_bits[e]);
        DUMPBITS(e)

        /* decode distance of block to copy */
        NEEDBITS((unsigned)bd)
        t = td + ((unsigned)b & md);
        while (1) {
          DUMPBITS(t->b)
          if ((e = t->e) < 32)
            break;
          if (IS_INVALID_CODE(e))
            return 1;
          e &= 31;
          NEEDBITS(e)
          t = t->v.t + ((unsigned)b & mask_bits[e]);
        }
        NEEDBITS(e)
        d = (unsigned)w - t->v.n - ((unsigned)b & mask_bits[e]);
        DUMPBITS(e)

        /* do the copy */
        do {
            e = (unsigned)(WSIZE -
                           ((d &= (unsigned)(WSIZE-1)) > (unsigned)w ?
                            (uint32_t)d : w));
          if ((uint32_t)e > n) e = (unsigned)n;
          n -= e;
#ifndef NOMEMCPY
          if ((unsigned)w - d >= e)
          /* (this test assumes unsigned comparison) */
          {
            memcpy(redirSlide + (unsigned)w, redirSlide + d, e);
            w += e;
            d += e;
          }
          else                  /* do it slowly to avoid memcpy() overlap */
#endif /* !NOMEMCPY */
            do {
              redirSlide[w++] = redirSlide[d++];
            } while (--e);
          if (w == WSIZE)
          {
            if ((retval = FLUSH(w)) != 0) goto cleanup_and_exit;
            w = 0;
          }
        } while (n);
        break;
      }

      if (e == 31)              /* it's the EOB signal */
      {
        /* sorry for this goto, but we have to exit two loops at once */
        goto cleanup_decode;
      }

      if (IS_INVALID_CODE(e))
        return 1;

      e &= 31;
      NEEDBITS(e)
      t = t->v.t + ((unsigned)b & mask_bits[e]);
    }
  }
cleanup_decode:

  /* restore the globals from the locals */
  G.wp = (unsigned)w;             /* restore global window pointer */
  G.bb = b;                       /* restore global bit buffer */
  G.bk = k;


cleanup_and_exit:
  /* done */
  return retval;
}

static int
inflate_stored(struct globals *Gp)
/* "decompress" an inflated type 0 (stored) block. */
{
  uint32_t w;           /* current window position (deflate64: up to 64k!) */
  unsigned n;           /* number of bytes in block */
  register uint32_t b;       /* bit buffer */
  register unsigned k;  /* number of bits in bit buffer */
  int retval = 0;       /* error code returned: initialized to "no error" */


  /* make local copies of globals */
  Trace((stderr, "\nstored block"));
  b = G.bb;                       /* initialize bit buffer */
  k = G.bk;
  w = G.wp;                       /* initialize window position */


  /* go to byte boundary */
  n = k & 7;
  DUMPBITS(n);


  /* get the length and its complement */
  NEEDBITS(16)
  n = ((unsigned)b & 0xffff);
  DUMPBITS(16)
  NEEDBITS(16)
  if (n != (unsigned)((~b) & 0xffff))
    return 1;                   /* error in compressed data */
  DUMPBITS(16)


  /* read and output the compressed data */
  while (n--)
  {
    NEEDBITS(8)
    redirSlide[w++] = (uint8_t)b;
    if (w == WSIZE)
    {
      if ((retval = FLUSH(w)) != 0) goto cleanup_and_exit;
      w = 0;
    }
    DUMPBITS(8)
  }


  /* restore the globals from the locals */
  G.wp = (unsigned)w;             /* restore global window pointer */
  G.bb = b;                       /* restore global bit buffer */
  G.bk = k;

cleanup_and_exit:
  return retval;
}


static int
inflate_fixed(struct globals *Gp)
/* decompress an inflated type 1 (fixed Huffman codes) block.  We should
   either replace this with a custom decoder, or at least precompute the
   Huffman tables. */
{
  /* if first time, set up tables for fixed blocks */
  Trace((stderr, "\nliteral block"));
  if (G.fixed_tl == NULL)
  {
    int i;                /* temporary variable */
    unsigned l[288];      /* length list for huft_build */

    /* literal table */
    for (i = 0; i < 144; i++)
      l[i] = 8;
    for (; i < 256; i++)
      l[i] = 9;
    for (; i < 280; i++)
      l[i] = 7;
    for (; i < 288; i++)          /* make a complete, but wrong code set */
      l[i] = 8;
    G.fixed_bl = 7;
    if ((i = huft_build(l, 288, 257, G.cplens, G.cplext,
                        &G.fixed_tl, &G.fixed_bl,
			Bits, Nob, Eob)) != 0)
    {
      G.fixed_tl = NULL;
      return i;
    }

    /* distance table */
    for (i = 0; i < MAXDISTS; i++)      /* make an incomplete code set */
      l[i] = 5;
    G.fixed_bd = 5;
    if ((i = huft_build(l, MAXDISTS, 0, cpdist, G.cpdext,
                        &G.fixed_td, &G.fixed_bd,
			Bits, Nob, Eob)) > 1)
    {
      huft_free(G.fixed_tl);
      G.fixed_td = G.fixed_tl = NULL;
      return i;
    }
  }

  /* decompress until an end-of-block code */
  return inflate_codes(&G, G.fixed_tl, G.fixed_td,
                             G.fixed_bl, G.fixed_bd);
}



static int inflate_dynamic(struct globals *Gp)
/* decompress an inflated type 2 (dynamic Huffman codes) block. */
{
  int i;                /* temporary variables */
  unsigned j;
  unsigned l;           /* last length */
  unsigned m;           /* mask for bit lengths table */
  unsigned n;           /* number of lengths to get */
  struct huft *tl;      /* literal/length code table */
  struct huft *td;      /* distance code table */
  int bl;               /* lookup bits for tl */
  int bd;               /* lookup bits for td */
  unsigned nb;          /* number of bit length codes */
  unsigned nl;          /* number of literal/length codes */
  unsigned nd;          /* number of distance codes */
  unsigned ll[MAXLITLENS+MAXDISTS]; /* lit./length and distance code lengths */
  register uint32_t b;       /* bit buffer */
  register unsigned k;  /* number of bits in bit buffer */
  int retval = 0;       /* error code returned: initialized to "no error" */


  /* make local bit buffer */
  Trace((stderr, "\ndynamic block"));
  b = G.bb;
  k = G.bk;


  /* read in table lengths */
  NEEDBITS(5)
  nl = 257 + ((unsigned)b & 0x1f);      /* number of literal/length codes */
  DUMPBITS(5)
  NEEDBITS(5)
  nd = 1 + ((unsigned)b & 0x1f);        /* number of distance codes */
  DUMPBITS(5)
  NEEDBITS(4)
  nb = 4 + ((unsigned)b & 0xf);         /* number of bit length codes */
  DUMPBITS(4)
  if (nl > MAXLITLENS || nd > MAXDISTS)
    return 1;                   /* bad lengths */


  /* read in bit-length-code lengths */
  for (j = 0; j < nb; j++)
  {
    NEEDBITS(3)
    ll[border[j]] = (unsigned)b & 7;
    DUMPBITS(3)
  }
  for (; j < 19; j++)
    ll[border[j]] = 0;


  /* build decoding table for trees--single level, 7 bit lookup */
  bl = 7;
  retval = huft_build(ll, 19, 19, NULL, NULL, &tl, &bl,
		  Bits, Nob, Eob);
  if (bl == 0)                  /* no bit lengths */
    retval = 1;
  if (retval)
  {
    if (retval == 1)
      huft_free(tl);
    return retval;              /* incomplete code set */
  }


  /* read in literal and distance code lengths */
  n = nl + nd;
  m = mask_bits[bl];
  i = l = 0;
  while ((unsigned)i < n)
  {
    NEEDBITS((unsigned)bl)
    j = (td = tl + ((unsigned)b & m))->b;
    DUMPBITS(j)
    j = td->v.n;
    if (j < 16)                 /* length of code in bits (0..15) */
      ll[i++] = l = j;          /* save last length in l */
    else if (j == 16)           /* repeat last length 3 to 6 times */
    {
      NEEDBITS(2)
      j = 3 + ((unsigned)b & 3);
      DUMPBITS(2)
      if ((unsigned)i + j > n)
        return 1;
      while (j--)
        ll[i++] = l;
    }
    else if (j == 17)           /* 3 to 10 zero length codes */
    {
      NEEDBITS(3)
      j = 3 + ((unsigned)b & 7);
      DUMPBITS(3)
      if ((unsigned)i + j > n)
        return 1;
      while (j--)
        ll[i++] = 0;
      l = 0;
    }
    else                        /* j == 18: 11 to 138 zero length codes */
    {
      NEEDBITS(7)
      j = 11 + ((unsigned)b & 0x7f);
      DUMPBITS(7)
      if ((unsigned)i + j > n)
        return 1;
      while (j--)
        ll[i++] = 0;
      l = 0;
    }
  }


  /* free decoding table for trees */
  huft_free(tl);


  /* restore the global bit buffer */
  G.bb = b;
  G.bk = k;


  /* build the decoding tables for literal/length and distance codes */
  bl = lbits;
  retval = huft_build(ll, nl, 257, G.cplens, G.cplext, &tl, &bl,
		  Bits, Nob, Eob);
  if (bl == 0)                  /* no literals or lengths */
    retval = 1;
  if (retval)
  {
    if (retval == 1) {
      /*if (!uO.qflag)
        MESSAGE((uint8_t *)"(incomplete l-tree)  ", 21L, 1);*/
      huft_free(tl);
    }
    return retval;              /* incomplete code set */
  }
  bd = dbits;
  retval = huft_build(ll + nl, nd, 0, cpdist, G.cpdext, &td, &bd,
		  Bits, Nob, Eob);
  if (retval == 1)
    retval = 0;
  if (bd == 0 && nl > 257)    /* lengths but no distances */
    retval = 1;
  if (retval)
  {
    if (retval == 1) {
      /*if (!uO.qflag)
        MESSAGE((uint8_t *)"(incomplete d-tree)  ", 21L, 1);*/
      huft_free(td);
    }
    huft_free(tl);
    return retval;
  }

  /* decompress until an end-of-block code */
  retval = inflate_codes(&G, tl, td, bl, bd);

cleanup_and_exit:
  /* free the decoding tables, return */
  huft_free(tl);
  huft_free(td);
  return retval;
}



static int inflate_block(struct globals *Gp, int *e)
/*int *e;*/             /* last block flag */
/* decompress an inflated block */
{
  unsigned t;           /* block type */
  register uint32_t b;       /* bit buffer */
  register unsigned k;  /* number of bits in bit buffer */
  int retval = 0;       /* error code returned: initialized to "no error" */


  /* make local bit buffer */
  b = G.bb;
  k = G.bk;


  /* read in last block bit */
  NEEDBITS(1)
  *e = (int)b & 1;
  DUMPBITS(1)


  /* read in block type */
  NEEDBITS(2)
  t = (unsigned)b & 3;
  DUMPBITS(2)


  /* restore the global bit buffer */
  G.bb = b;
  G.bk = k;


  /* inflate that block type */
  if (t == 2)
    return inflate_dynamic(&G);
  if (t == 0)
    return inflate_stored(&G);
  if (t == 1)
    return inflate_fixed(&G);


  /* bad block type */
  retval = 2;

cleanup_and_exit:
  return retval;
}

#undef	G

int
zipinflate(struct file *f, const char *tgt, int tfd, int doswap, uint32_t *crc)
/* decompress an inflated entry */
{
  struct globals G;
  int e = 0;            /* last block flag */
  int r;                /* result code */
  int is_defl64;
#ifdef DEBUG
  unsigned h = 0;       /* maximum struct huft's malloc'ed */
#endif

  is_defl64 = f->f_cmethod == C_ENHDEFLD;
  memset(&G, 0, sizeof G);
  G.tgt = tgt;
  G.tfd = tfd;
  G.doswap = doswap;
  G.crc = crc;
  G.zsize = G.uzsize = f->f_csize;
  G.ucsize = f->f_st.st_size;
  /* initialize window, bit buffer */
  G.wp = 0;
  G.bk = 0;
  G.bb = 0;

  if (is_defl64) {
    G.cplens = cplens64;
    G.cplext = cplext64;
    G.cpdext = cpdext64;
    G.fixed_tl = G.fixed_tl64;
    G.fixed_bl = G.fixed_bl64;
    G.fixed_td = G.fixed_td64;
    G.fixed_bd = G.fixed_bd64;
  } else {
    G.cplens = cplens32;
    G.cplext = cplext32;
    G.cpdext = cpdext32;
    G.fixed_tl = G.fixed_tl32;
    G.fixed_bl = G.fixed_bl32;
    G.fixed_td = G.fixed_td32;
    G.fixed_bd = G.fixed_bd32;
  }

  /* decompress until the last block */
  do {
#ifdef DEBUG
    G.hufts = 0;
#endif
    if ((r = inflate_block(&G, &e)) != 0) {
      if ((f->f_gflag & FG_DESC) == 0)
      	while (G.uzsize > 0)
	      	NEXTBYTE;
      msg(3, 0, "compression error on \"%s\"\n", f->f_name);
      return -1;
    }
#ifdef DEBUG
    if (G.hufts > h)
      h = G.hufts;
#endif
  } while (!e);

  Trace((stderr, "\n%u bytes in Huffman tables (%u/entry)\n",
         h * (unsigned)sizeof(struct huft), (unsigned)sizeof(struct huft)));

  if (is_defl64) {
    G.fixed_tl64 = G.fixed_tl;
    G.fixed_bl64 = G.fixed_bl;
    G.fixed_td64 = G.fixed_td;
    G.fixed_bd64 = G.fixed_bd;
  } else {
    G.fixed_tl32 = G.fixed_tl;
    G.fixed_bl32 = G.fixed_bl;
    G.fixed_td32 = G.fixed_td;
    G.fixed_bd32 = G.fixed_bd;
  }

  /* flush out redirSlide and return (success, unless final FLUSH failed) */
  (FLUSH(G.wp));
  if (f->f_gflag & FG_DESC)
	  bunread((char *)G.inptr, G.incnt);
  return G.status;
}
