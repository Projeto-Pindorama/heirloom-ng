/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, May 2003.
 *
 * Derived from unzip 5.40.
 *
 * Sccsid @(#)explode.c	1.6 (gritter) 9/30/03
 */
/* explode.c -- put in the public domain by Mark Adler
   version c15, 6 July 1996 */


/* You can do whatever you like with this source file, though I would
   prefer that if you modify it and redistribute it that you include
   comments to that effect with your name and the date.  Thank you.

   History:
   vers    date          who           what
   ----  ---------  --------------  ------------------------------------
    c1   30 Mar 92  M. Adler        explode that uses huft_build from inflate
                                    (this gives over a 70% speed improvement
                                    over the original unimplode.c, which
                                    decoded a bit at a time)
    c2    4 Apr 92  M. Adler        fixed bug for file sizes a multiple of 32k.
    c3   10 Apr 92  M. Adler        added a little memory tracking if DEBUG
    c4   11 Apr 92  M. Adler        added NOMEMCPY do kill use of memcpy()
    c5   21 Apr 92  M. Adler        added the WSIZE #define to allow reducing
                                    the 32K window size for specialized
                                    applications.
    c6   31 May 92  M. Adler        added typecasts to eliminate some warnings
    c7   27 Jun 92  G. Roelofs      added more typecasts.
    c8   17 Oct 92  G. Roelofs      changed ULONG/UWORD/byte to ulg/ush/uch.
    c9   19 Jul 93  J. Bush         added more typecasts (to return values);
                                    made l[256] array static for Amiga.
    c10   8 Oct 93  G. Roelofs      added used_csize for diagnostics; added
                                    buf and unshrink arguments to flush();
                                    undef'd various macros at end for Turbo C;
                                    removed NEXTBYTE macro (now in unzip.h)
                                    and bytebuf variable (not used); changed
                                    memset() to memzero().
    c11   9 Jan 94  M. Adler        fixed incorrect used_csize calculation.
    c12   9 Apr 94  G. Roelofs      fixed split comments on preprocessor lines
                                    to avoid bug in Encore compiler.
    c13  25 Aug 94  M. Adler        fixed distance-length comment (orig c9 fix)
    c14  22 Nov 95  S. Maxwell      removed unnecessary "static" on auto array
    c15   6 Jul 96  W. Haidinger    added ulg typecasts to flush() calls.
    c16   8 Feb 98  C. Spieler      added ZCONST modifiers to const tables
                                    and #ifdef DEBUG around debugging code.
    c16b 25 Mar 98  C. Spieler      modified DLL code for slide redirection.

         23 May 03  Gunnar Ritter   use cpio structures; C99 conversion.
 */


/*
   Explode imploded (PKZIP method 6 compressed) data.  This compression
   method searches for as much of the current string of bytes (up to a length
   of ~320) in the previous 4K or 8K bytes.  If it doesn't find any matches
   (of at least length 2 or 3), it codes the next byte.  Otherwise, it codes
   the length of the matched string and its distance backwards from the
   current position.  Single bytes ("literals") are preceded by a one (a
   single bit) and are either uncoded (the eight bits go directly into the
   compressed stream for a total of nine bits) or Huffman coded with a
   supplied literal code tree.  If literals are coded, then the minimum match
   length is three, otherwise it is two.

   There are therefore four kinds of imploded streams: 8K search with coded
   literals (min match = 3), 4K search with coded literals (min match = 3),
   8K with uncoded literals (min match = 2), and 4K with uncoded literals
   (min match = 2).  The kind of stream is identified in two bits of a
   general purpose bit flag that is outside of the compressed stream.

   Distance-length pairs for matched strings are preceded by a zero bit (to
   distinguish them from literals) and are always coded.  The distance comes
   first and is either the low six (4K) or low seven (8K) bits of the
   distance (uncoded), followed by the high six bits of the distance coded.
   Then the length is six bits coded (0..63 + min match length), and if the
   maximum such length is coded, then it's followed by another eight bits
   (uncoded) to be added to the coded length.  This gives a match length
   range of 2..320 or 3..321 bytes.

   The literal, length, and distance codes are all represented in a slightly
   compressed form themselves.  What is sent are the lengths of the codes for
   each value, which is sufficient to construct the codes.  Each byte of the
   code representation is the code length (the low four bits representing
   1..16), and the number of values sequentially with that length (the high
   four bits also representing 1..16).  There are 256 literal code values (if
   literals are coded), 64 length code values, and 64 distance code values,
   in that order at the beginning of the compressed stream.  Each set of code
   values is preceded (redundantly) with a byte indicating how many bytes are
   in the code description that follows, in the range 1..256.

   The codes themselves are decoded using tables made by huft_build() from
   the bit lengths.  That routine and its comments are in the inflate.c
   module.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cpio.h"
#include "unzip.h"  /* must supply slide[] (uint8_t) array and NEXTBYTE macro */

/* routines here */
static int get_tree(struct globals *, unsigned *l, unsigned n);
static int explode_lit8(struct globals *, struct huft *tb, struct huft *tl,
                            struct huft *td, int bb, int bl, int bd);
static int explode_lit4(struct globals *, struct huft *tb, struct huft *tl,
                            struct huft *td, int bb, int bl, int bd);
static int explode_nolit8(struct globals *, struct huft *tl, struct huft *td,
                              int bl, int bd);
static int explode_nolit4(struct globals *, struct huft *tl, struct huft *td,
                              int bl, int bd);

/* The implode algorithm uses a sliding 4K or 8K byte window on the
   uncompressed stream to find repeated byte strings.  This is implemented
   here as a circular buffer.  The index is updated simply by incrementing
   and then and'ing with 0x0fff (4K-1) or 0x1fff (8K-1).  Here, the 32K
   buffer of inflate is used, and it works just as well to always have
   a 32K circular buffer, so the index is anded with 0x7fff.  This is
   done to allow the window to also be used as the output buffer. */
/* This must be supplied in an external module useable like
   "uint8_t slide[8192];" or "uint8_t *slide;", where the latter would
   be malloc'ed.  In unzip, slide[] is actually a 32K area for use by
   inflate, which uses a 32K sliding window.
 */


/* Tables for length and distance */
static const uint16_t cplen2[] =
        {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
        18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34,
        35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65};
static const uint16_t cplen3[] =
        {3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
        19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
        36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52,
        53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66};
static const uint8_t extra[] =
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        8};
static const uint16_t cpdist4[] =
        {1, 65, 129, 193, 257, 321, 385, 449, 513, 577, 641, 705,
        769, 833, 897, 961, 1025, 1089, 1153, 1217, 1281, 1345, 1409, 1473,
        1537, 1601, 1665, 1729, 1793, 1857, 1921, 1985, 2049, 2113, 2177,
        2241, 2305, 2369, 2433, 2497, 2561, 2625, 2689, 2753, 2817, 2881,
        2945, 3009, 3073, 3137, 3201, 3265, 3329, 3393, 3457, 3521, 3585,
        3649, 3713, 3777, 3841, 3905, 3969, 4033};
static const uint16_t cpdist8[] =
        {1, 129, 257, 385, 513, 641, 769, 897, 1025, 1153, 1281,
        1409, 1537, 1665, 1793, 1921, 2049, 2177, 2305, 2433, 2561, 2689,
        2817, 2945, 3073, 3201, 3329, 3457, 3585, 3713, 3841, 3969, 4097,
        4225, 4353, 4481, 4609, 4737, 4865, 4993, 5121, 5249, 5377, 5505,
        5633, 5761, 5889, 6017, 6145, 6273, 6401, 6529, 6657, 6785, 6913,
        7041, 7169, 7297, 7425, 7553, 7681, 7809, 7937, 8065};


/* Macros for inflate() bit peeking and grabbing.
   The usage is:

        NEEDBITS(j)
        x = b & mask_bits[j];
        DUMPBITS(j)

   where NEEDBITS makes sure that b has at least j bits in it, and
   DUMPBITS removes the bits from b.  The macros use the variable k
   for the number of bits in b.  Normally, b and k are register
   variables for speed.
 */

#define NEEDBITS(n) {while(k<(n)){b|=((uint32_t)NEXTBYTE)<<k;k+=8;}}
#define DUMPBITS(n) {b>>=(n);k-=(n);}

#define	Bits	16
#define	Nob	16
#define	Eob	15

#define	G	(*Gp)

static int
get_tree(struct globals *Gp, unsigned *l, unsigned n)
/*unsigned *l;*/          /* bit lengths */
/*unsigned n;*/           /* number expected */
/* Get the bit lengths for a code representation from the compressed
   stream.  If get_tree() returns 4, then there is an error in the data.
   Otherwise zero is returned. */
{
  unsigned i;           /* bytes remaining in list */
  unsigned k;           /* lengths entered */
  unsigned j;           /* number of codes */
  unsigned b;           /* bit length for those codes */


  /* get bit lengths */
  i = NEXTBYTE + 1;                     /* length/count pairs to read */
  k = 0;                                /* next code */
  do {
    b = ((j = NEXTBYTE) & 0xf) + 1;     /* bits in code (1..16) */
    j = ((j & 0xf0) >> 4) + 1;          /* codes with those bits (1..16) */
    if (k + j > n)
      return 4;                         /* don't overflow l[] */
    do {
      l[k++] = b;
    } while (--j);
  } while (--i);
  return k != n ? 4 : 0;                /* should have read n of them */
}



static int
explode_lit8(struct globals *Gp,
		struct huft *tb, struct huft *tl, struct huft *td,
		int bb, int bl, int bd)
/*struct huft *tb, *tl, *td;*/    /* literal, length, and distance tables */
/*int bb, bl, bd;*/               /* number of bits decoded by those */
/* Decompress the imploded data using coded literals and an 8K sliding
   window. */
{
  long s;               /* bytes to decompress */
  register unsigned e;  /* table entry flag/number of extra bits */
  unsigned n, d;        /* length and index for copy */
  unsigned w;           /* current window position */
  struct huft *t;       /* pointer to table entry */
  unsigned mb, ml, md;  /* masks for bb, bl, and bd bits */
  register uint32_t b;  /* bit buffer */
  register unsigned k;  /* number of bits in bit buffer */
  unsigned u;           /* true if unflushed */


  /* explode the coded data */
  b = k = w = 0;                /* initialize bit buffer, window */
  u = 1;                        /* buffer unflushed */
  mb = mask_bits[bb];           /* precompute masks for speed */
  ml = mask_bits[bl];
  md = mask_bits[bd];
  s = G.ucsize;
  while (s > 0)                 /* do until ucsize bytes uncompressed */
  {
    NEEDBITS(1)
    if (b & 1)                  /* then literal--decode it */
    {
      DUMPBITS(1)
      s--;
      NEEDBITS((unsigned)bb)    /* get coded literal */
      if ((e = (t = tb + ((~(unsigned)b) & mb))->e) > 16)
        do {
          if (e == 99)
            return 1;
          DUMPBITS(t->b)
          e -= 16;
          NEEDBITS(e)
        } while ((e = (t = t->v.t + ((~(unsigned)b) & mask_bits[e]))->e) > 16);
      DUMPBITS(t->b)
      redirSlide[w++] = (uint8_t)t->v.n;
      if (w == WSIZE)
      {
        flush(&G, redirSlide, (uint32_t)w);
        w = u = 0;
      }
    }
    else                        /* else distance/length */
    {
      DUMPBITS(1)
      NEEDBITS(7)               /* get distance low bits */
      d = (unsigned)b & 0x7f;
      DUMPBITS(7)
      NEEDBITS((unsigned)bd)    /* get coded distance high bits */
      if ((e = (t = td + ((~(unsigned)b) & md))->e) > 16)
        do {
          if (e == 99)
            return 1;
          DUMPBITS(t->b)
          e -= 16;
          NEEDBITS(e)
        } while ((e = (t = t->v.t + ((~(unsigned)b) & mask_bits[e]))->e) > 16);
      DUMPBITS(t->b)
      d = w - d - t->v.n;       /* construct offset */
      NEEDBITS((unsigned)bl)    /* get coded length */
      if ((e = (t = tl + ((~(unsigned)b) & ml))->e) > 16)
        do {
          if (e == 99)
            return 1;
          DUMPBITS(t->b)
          e -= 16;
          NEEDBITS(e)
        } while ((e = (t = t->v.t + ((~(unsigned)b) & mask_bits[e]))->e) > 16);
      DUMPBITS(t->b)
      n = t->v.n;
      if (e)                    /* get length extra bits */
      {
        NEEDBITS(8)
        n += (unsigned)b & 0xff;
        DUMPBITS(8)
      }

      /* do the copy */
      s -= n;
      do {
        n -= (e = (e = WSIZE - ((d &= WSIZE-1) > w ? d : w)) > n ? n : e);
        if (u && w <= d)
        {
          memset(redirSlide + w, 0, e);
          w += e;
          d += e;
        }
        else
#ifndef NOMEMCPY
          if (w - d >= e)       /* (this test assumes unsigned comparison) */
          {
            memcpy(redirSlide + w, redirSlide + d, e);
            w += e;
            d += e;
          }
          else                  /* do it slow to avoid memcpy() overlap */
#endif /* !NOMEMCPY */
            do {
              redirSlide[w++] = redirSlide[d++];
            } while (--e);
        if (w == WSIZE)
        {
          flush(&G, redirSlide, (uint32_t)w);
          w = u = 0;
        }
      } while (n);
    }
  }

  /* flush out redirSlide */
  flush(&G, redirSlide, (uint32_t)w);
  if (G.csize + G.incnt + (k >> 3))   /* should have read csize bytes, but */
  {                        /* sometimes read one too many:  k>>3 compensates */
    /*G.used_csize = G.zsize - G.csize - G.incnt - (k >> 3);*/
    return 5;
  }
  return 0;
}



static int
explode_lit4(struct globals *Gp,
		struct huft *tb, struct huft *tl, struct huft *td,
		int bb, int bl, int bd)
/*struct huft *tb, *tl, *td;*/    /* literal, length, and distance tables */
/*int bb, bl, bd;*/               /* number of bits decoded by those */
/* Decompress the imploded data using coded literals and a 4K sliding
   window. */
{
  long s;               /* bytes to decompress */
  register unsigned e;  /* table entry flag/number of extra bits */
  unsigned n, d;        /* length and index for copy */
  unsigned w;           /* current window position */
  struct huft *t;       /* pointer to table entry */
  unsigned mb, ml, md;  /* masks for bb, bl, and bd bits */
  register uint32_t b;  /* bit buffer */
  register unsigned k;  /* number of bits in bit buffer */
  unsigned u;           /* true if unflushed */


  /* explode the coded data */
  b = k = w = 0;                /* initialize bit buffer, window */
  u = 1;                        /* buffer unflushed */
  mb = mask_bits[bb];           /* precompute masks for speed */
  ml = mask_bits[bl];
  md = mask_bits[bd];
  s = G.ucsize;
  while (s > 0)                 /* do until ucsize bytes uncompressed */
  {
    NEEDBITS(1)
    if (b & 1)                  /* then literal--decode it */
    {
      DUMPBITS(1)
      s--;
      NEEDBITS((unsigned)bb)    /* get coded literal */
      if ((e = (t = tb + ((~(unsigned)b) & mb))->e) > 16)
        do {
          if (e == 99)
            return 1;
          DUMPBITS(t->b)
          e -= 16;
          NEEDBITS(e)
        } while ((e = (t = t->v.t + ((~(unsigned)b) & mask_bits[e]))->e) > 16);
      DUMPBITS(t->b)
      redirSlide[w++] = (uint8_t)t->v.n;
      if (w == WSIZE)
      {
        flush(&G, redirSlide, (uint32_t)w);
        w = u = 0;
      }
    }
    else                        /* else distance/length */
    {
      DUMPBITS(1)
      NEEDBITS(6)               /* get distance low bits */
      d = (unsigned)b & 0x3f;
      DUMPBITS(6)
      NEEDBITS((unsigned)bd)    /* get coded distance high bits */
      if ((e = (t = td + ((~(unsigned)b) & md))->e) > 16)
        do {
          if (e == 99)
            return 1;
          DUMPBITS(t->b)
          e -= 16;
          NEEDBITS(e)
        } while ((e = (t = t->v.t + ((~(unsigned)b) & mask_bits[e]))->e) > 16);
      DUMPBITS(t->b)
      d = w - d - t->v.n;       /* construct offset */
      NEEDBITS((unsigned)bl)    /* get coded length */
      if ((e = (t = tl + ((~(unsigned)b) & ml))->e) > 16)
        do {
          if (e == 99)
            return 1;
          DUMPBITS(t->b)
          e -= 16;
          NEEDBITS(e)
        } while ((e = (t = t->v.t + ((~(unsigned)b) & mask_bits[e]))->e) > 16);
      DUMPBITS(t->b)
      n = t->v.n;
      if (e)                    /* get length extra bits */
      {
        NEEDBITS(8)
        n += (unsigned)b & 0xff;
        DUMPBITS(8)
      }

      /* do the copy */
      s -= n;
      do {
        n -= (e = (e = WSIZE - ((d &= WSIZE-1) > w ? d : w)) > n ? n : e);
        if (u && w <= d)
        {
          memset(redirSlide + w, 0, e);
          w += e;
          d += e;
        }
        else
#ifndef NOMEMCPY
          if (w - d >= e)       /* (this test assumes unsigned comparison) */
          {
            memcpy(redirSlide + w, redirSlide + d, e);
            w += e;
            d += e;
          }
          else                  /* do it slow to avoid memcpy() overlap */
#endif /* !NOMEMCPY */
            do {
              redirSlide[w++] = redirSlide[d++];
            } while (--e);
        if (w == WSIZE)
        {
          flush(&G, redirSlide, (uint32_t)w);
          w = u = 0;
        }
      } while (n);
    }
  }

  /* flush out redirSlide */
  flush(&G, redirSlide, (uint32_t)w);
  if (G.csize + G.incnt + (k >> 3))   /* should have read csize bytes, but */
  {                        /* sometimes read one too many:  k>>3 compensates */
    /*G.used_csize = G.zsize - G.csize - G.incnt - (k >> 3);*/
    return 5;
  }
  return 0;
}



static int
explode_nolit8(struct globals *Gp,
		struct huft *tl, struct huft *td, int bl, int bd)
/*struct huft *tl, *td;*/ /* length and distance decoder tables */
/*int bl, bd;*/           /* number of bits decoded by tl[] and td[] */
/* Decompress the imploded data using uncoded literals and an 8K sliding
   window. */
{
  long s;               /* bytes to decompress */
  register unsigned e;  /* table entry flag/number of extra bits */
  unsigned n, d;        /* length and index for copy */
  unsigned w;           /* current window position */
  struct huft *t;       /* pointer to table entry */
  unsigned ml, md;      /* masks for bl and bd bits */
  register uint32_t b;  /* bit buffer */
  register unsigned k;  /* number of bits in bit buffer */
  unsigned u;           /* true if unflushed */


  /* explode the coded data */
  b = k = w = 0;                /* initialize bit buffer, window */
  u = 1;                        /* buffer unflushed */
  ml = mask_bits[bl];           /* precompute masks for speed */
  md = mask_bits[bd];
  s = G.ucsize;
  while (s > 0)                 /* do until ucsize bytes uncompressed */
  {
    NEEDBITS(1)
    if (b & 1)                  /* then literal--get eight bits */
    {
      DUMPBITS(1)
      s--;
      NEEDBITS(8)
      redirSlide[w++] = (uint8_t)b;
      if (w == WSIZE)
      {
        flush(&G, redirSlide, (uint32_t)w);
        w = u = 0;
      }
      DUMPBITS(8)
    }
    else                        /* else distance/length */
    {
      DUMPBITS(1)
      NEEDBITS(7)               /* get distance low bits */
      d = (unsigned)b & 0x7f;
      DUMPBITS(7)
      NEEDBITS((unsigned)bd)    /* get coded distance high bits */
      if ((e = (t = td + ((~(unsigned)b) & md))->e) > 16)
        do {
          if (e == 99)
            return 1;
          DUMPBITS(t->b)
          e -= 16;
          NEEDBITS(e)
        } while ((e = (t = t->v.t + ((~(unsigned)b) & mask_bits[e]))->e) > 16);
      DUMPBITS(t->b)
      d = w - d - t->v.n;       /* construct offset */
      NEEDBITS((unsigned)bl)    /* get coded length */
      if ((e = (t = tl + ((~(unsigned)b) & ml))->e) > 16)
        do {
          if (e == 99)
            return 1;
          DUMPBITS(t->b)
          e -= 16;
          NEEDBITS(e)
        } while ((e = (t = t->v.t + ((~(unsigned)b) & mask_bits[e]))->e) > 16);
      DUMPBITS(t->b)
      n = t->v.n;
      if (e)                    /* get length extra bits */
      {
        NEEDBITS(8)
        n += (unsigned)b & 0xff;
        DUMPBITS(8)
      }

      /* do the copy */
      s -= n;
      do {
        n -= (e = (e = WSIZE - ((d &= WSIZE-1) > w ? d : w)) > n ? n : e);
        if (u && w <= d)
        {
          memset(redirSlide + w, 0, e);
          w += e;
          d += e;
        }
        else
#ifndef NOMEMCPY
          if (w - d >= e)       /* (this test assumes unsigned comparison) */
          {
            memcpy(redirSlide + w, redirSlide + d, e);
            w += e;
            d += e;
          }
          else                  /* do it slow to avoid memcpy() overlap */
#endif /* !NOMEMCPY */
            do {
              redirSlide[w++] = redirSlide[d++];
            } while (--e);
        if (w == WSIZE)
        {
          flush(&G, redirSlide, (uint32_t)w);
          w = u = 0;
        }
      } while (n);
    }
  }

  /* flush out redirSlide */
  flush(&G, redirSlide, (uint32_t)w);
  if (G.csize + G.incnt + (k >> 3))   /* should have read csize bytes, but */
  {                        /* sometimes read one too many:  k>>3 compensates */
    /*G.used_csize = G.zsize - G.csize - G.incnt - (k >> 3);*/
    return 5;
  }
  return 0;
}



static int
explode_nolit4(struct globals *Gp,
		struct huft *tl, struct huft *td, int bl, int bd)
/*struct huft *tl, *td;*/ /* length and distance decoder tables */
/*int bl, bd;*/           /* number of bits decoded by tl[] and td[] */
/* Decompress the imploded data using uncoded literals and a 4K sliding
   window. */
{
  long s;               /* bytes to decompress */
  register unsigned e;  /* table entry flag/number of extra bits */
  unsigned n, d;        /* length and index for copy */
  unsigned w;           /* current window position */
  struct huft *t;       /* pointer to table entry */
  unsigned ml, md;      /* masks for bl and bd bits */
  register uint32_t b;  /* bit buffer */
  register unsigned k;  /* number of bits in bit buffer */
  unsigned u;           /* true if unflushed */


  /* explode the coded data */
  b = k = w = 0;                /* initialize bit buffer, window */
  u = 1;                        /* buffer unflushed */
  ml = mask_bits[bl];           /* precompute masks for speed */
  md = mask_bits[bd];
  s = G.ucsize;
  while (s > 0)                 /* do until ucsize bytes uncompressed */
  {
    NEEDBITS(1)
    if (b & 1)                  /* then literal--get eight bits */
    {
      DUMPBITS(1)
      s--;
      NEEDBITS(8)
      redirSlide[w++] = (uint8_t)b;
      if (w == WSIZE)
      {
        flush(&G, redirSlide, (uint32_t)w);
        w = u = 0;
      }
      DUMPBITS(8)
    }
    else                        /* else distance/length */
    {
      DUMPBITS(1)
      NEEDBITS(6)               /* get distance low bits */
      d = (unsigned)b & 0x3f;
      DUMPBITS(6)
      NEEDBITS((unsigned)bd)    /* get coded distance high bits */
      if ((e = (t = td + ((~(unsigned)b) & md))->e) > 16)
        do {
          if (e == 99)
            return 1;
          DUMPBITS(t->b)
          e -= 16;
          NEEDBITS(e)
        } while ((e = (t = t->v.t + ((~(unsigned)b) & mask_bits[e]))->e) > 16);
      DUMPBITS(t->b)
      d = w - d - t->v.n;       /* construct offset */
      NEEDBITS((unsigned)bl)    /* get coded length */
      if ((e = (t = tl + ((~(unsigned)b) & ml))->e) > 16)
        do {
          if (e == 99)
            return 1;
          DUMPBITS(t->b)
          e -= 16;
          NEEDBITS(e)
        } while ((e = (t = t->v.t + ((~(unsigned)b) & mask_bits[e]))->e) > 16);
      DUMPBITS(t->b)
      n = t->v.n;
      if (e)                    /* get length extra bits */
      {
        NEEDBITS(8)
        n += (unsigned)b & 0xff;
        DUMPBITS(8)
      }

      /* do the copy */
      s -= n;
      do {
        n -= (e = (e = WSIZE - ((d &= WSIZE-1) > w ? d : w)) > n ? n : e);
        if (u && w <= d)
        {
          memset(redirSlide + w, 0, e);
          w += e;
          d += e;
        }
        else
#ifndef NOMEMCPY
          if (w - d >= e)       /* (this test assumes unsigned comparison) */
          {
            memcpy(redirSlide + w, redirSlide + d, e);
            w += e;
            d += e;
          }
          else                  /* do it slow to avoid memcpy() overlap */
#endif /* !NOMEMCPY */
            do {
              redirSlide[w++] = redirSlide[d++];
            } while (--e);
        if (w == WSIZE)
        {
          flush(&G, redirSlide, (uint32_t)w);
          w = u = 0;
        }
      } while (n);
    }
  }

  /* flush out redirSlide */
  flush(&G, redirSlide, (uint32_t)w);
  if (G.csize + G.incnt + (k >> 3))   /* should have read csize bytes, but */
  {                        /* sometimes read one too many:  k>>3 compensates */
    /*G.used_csize = G.zsize - G.csize - G.incnt - (k >> 3);*/
    return 5;
  }
  return 0;
}

#undef	G

int
zipexplode(struct file *f, const char *tgt, int tfd, int doswap, uint32_t *crc)
/* Explode an imploded compressed stream.  Based on the general purpose
   bit flag, decide on coded or uncoded literals, and an 8K or 4K sliding
   window.  Construct the literal (if any), length, and distance codes and
   the tables needed to decode them (using huft_build() from inflate.c),
   and call the appropriate routine for the type of data in the remainder
   of the stream.  The four routines are nearly identical, differing only
   in whether the literal is decoded or simply read in, and in how many
   bits are read in, uncoded, for the low distance bits. */
{
  struct globals G;
  unsigned r;           /* return codes */
  struct huft *tb;      /* literal code table */
  struct huft *tl;      /* length code table */
  struct huft *td;      /* distance code table */
  int bb;               /* bits for tb */
  int bl;               /* bits for tl */
  int bd;               /* bits for td */
  unsigned l[256];      /* bit lengths for codes */

  memset(&G, 0, sizeof G);
  G.tgt = tgt;
  G.tfd = tfd;
  G.doswap = doswap;
  G.crc = crc;
  G.zsize = G.uzsize = f->f_csize;
  G.ucsize = f->f_st.st_size;

  /* Tune base table sizes.  Note: I thought that to truly optimize speed,
     I would have to select different bl, bd, and bb values for different
     compressed file sizes.  I was surprised to find out that the values of
     7, 7, and 9 worked best over a very wide range of sizes, except that
     bd = 8 worked marginally better for large compressed sizes. */
  bl = 7;
  bd = (G.csize + G.incnt) > 200000L ? 8 : 7;


  /* With literal tree--minimum match length is 3 */
#ifdef DEBUG
  G.hufts = 0;                    /* initialize huft's malloc'ed */
#endif
  if (f->f_gflag & FG_BIT2)
  {
    bb = 9;                     /* base table size for literals */
    if ((r = get_tree(&G, l, 256)) != 0)
      goto err;
    if ((r = huft_build(l, 256, 256, NULL, NULL, &tb, &bb,
		Bits, Nob, Eob)) != 0)
    {
      if (r == 1)
        huft_free(tb);
      goto err;
    }
    if ((r = get_tree(&G, l, 64)) != 0)
      goto err;
    if ((r = huft_build(l, 64, 0, cplen3, extra, &tl, &bl,
		Bits, Nob, Eob)) != 0)
    {
      if (r == 1)
        huft_free(tl);
      huft_free(tb);
      goto err;
    }
    if ((r = get_tree(&G, l, 64)) != 0)
      goto err;
    if (f->f_gflag & FG_BIT1)      /* true if 8K */
    {
      if ((r = huft_build(l, 64, 0, cpdist8, extra, &td, &bd,
		Bits, Nob, Eob)) != 0)
      {
        if (r == 1)
          huft_free(td);
        huft_free(tl);
        huft_free(tb);
        goto err;
      }
      r = explode_lit8(&G, tb, tl, td, bb, bl, bd);
    }
    else                                        /* else 4K */
    {
      if ((r = huft_build(l, 64, 0, cpdist4, extra, &td, &bd,
		Bits, Nob, Eob)) != 0)
      {
        if (r == 1)
          huft_free(td);
        huft_free(tl);
        huft_free(tb);
        goto err;
      }
      r = explode_lit4(&G, tb, tl, td, bb, bl, bd);
    }
    huft_free(td);
    huft_free(tl);
    huft_free(tb);
  }
  else


  /* No literal tree--minimum match length is 2 */
  {
    if ((r = get_tree(&G, l, 64)) != 0)
      goto err;
    if ((r = huft_build(l, 64, 0, cplen2, extra, &tl, &bl,
		Bits, Nob, Eob)) != 0)
    {
      if (r == 1)
        huft_free(tl);
      goto err;
    }
    if ((r = get_tree(&G, l, 64)) != 0)
      goto err;
    if (f->f_gflag & FG_BIT1)      /* true if 8K */
    {
      if ((r = huft_build(l, 64, 0, cpdist8, extra, &td, &bd,
		Bits, Nob, Eob)) != 0)
      {
        if (r == 1)
          huft_free(td);
        huft_free(tl);
        goto err;
      }
      r = explode_nolit8(&G, tl, td, bl, bd);
    }
    else                                        /* else 4K */
    {
      if ((r = huft_build(l, 64, 0, cpdist4, extra, &td, &bd,
		Bits, Nob, Eob)) != 0)
      {
        if (r == 1)
          huft_free(td);
        huft_free(tl);
        goto err;
      }
      r = explode_nolit4(&G, tl, td, bl, bd);
    }
    huft_free(td);
    huft_free(tl);
  }
  Trace((stderr, "<%u > ", G.hufts));
err:
  switch (r) {
  case 0:
	  break;
  case 5:
	  while (G.uzsize > 0)
		  NEXTBYTE;
	  /*FALLTHRU*/
  default:
	  msg(3, 0, "compression error on \"%s\"\n", f->f_name);
  }
  return r || G.status ? -1 : 0;
}

/* The following code is derived from: */

/* inflate.c -- put in the public domain by Mark Adler
   version c16b, 29 March 1998 */

/* If BMAX needs to be larger than 16, then h and x[] should be uint32_t. */
#define BMAX 16         /* maximum bit length of any code (16 for explode) */
#define N_MAX 288       /* maximum number of codes in any set */


int
huft_build(const unsigned *b, unsigned n, unsigned s,
		const uint16_t *d, const uint8_t *e,
		struct huft **t, int *m,
		int bits, int nob, int eob)
/*const unsigned *b;*/ /* code lengths in bits (all assumed <= BMAX) */
/*unsigned n;*/         /* number of codes (assumed <= N_MAX) */
/*unsigned s;*/         /* number of simple-valued codes (0..s-1) */
/*const uint16_t *d;*/  /* list of base values for non-simple codes */
/*const uint16_t *e;*/  /* list of extra bits for non-simple codes */
/*struct huft **t;*/    /* result: starting table */
/*int *m;*/             /* maximum lookup bits, returns actual */
/* Given a list of code lengths and a maximum table size, make a set of
   tables to decode that set of codes.  Return zero on success, one if
   the given code set is incomplete (the tables are still built in this
   case), two if the input is invalid (all zero length codes or an
   oversubscribed set of lengths), and three if not enough memory.
   The code with value 256 is special, and the tables are constructed
   so that no bits beyond that code are fetched when that code is
   decoded. */
{
  unsigned a;                   /* counter for codes of length k */
  unsigned c[BMAX+1];           /* bit length count table */
  unsigned el;                  /* length of EOB code (value 256) */
  unsigned f;                   /* i repeats in table every f entries */
  int g;                        /* maximum code length */
  int h;                        /* table level */
  register unsigned i;          /* counter, current code */
  register unsigned j;          /* counter */
  register int k;               /* number of bits in current code */
  int lx[BMAX+1];               /* memory for l[-1..BMAX-1] */
  int *l = lx+1;                /* stack of bits per table */
  register unsigned *p;         /* pointer into c[], b[], or v[] */
  register struct huft *q;      /* points to current table */
  struct huft r;                /* table entry for structure assignment */
  struct huft *u[BMAX];         /* table stack */
  unsigned v[N_MAX];            /* values in order of bit length */
  register int w;               /* bits before this table == (l * h) */
  unsigned x[BMAX+1];           /* bit offsets, then code stack */
  unsigned *xp;                 /* pointer into x */
  int y;                        /* number of dummy codes added */
  unsigned z;                   /* number of entries in current table */


  /* Generate counts for each bit length */
  el = n > 256 ? b[256] : BMAX; /* set length of EOB code, if any */
  memset(c, 0, sizeof c);
  p = (unsigned *)b;  i = n;
  do {
    c[*p]++; p++;               /* assume all entries <= BMAX */
  } while (--i);
  if (c[0] == n)                /* null input--all zero length codes */
  {
    *t = NULL;
    *m = 0;
    return 0;
  }


  /* Find minimum and maximum length, bound *m by those */
  for (j = 1; j <= BMAX; j++)
    if (c[j])
      break;
  k = j;                        /* minimum code length */
  if ((unsigned)*m < j)
    *m = j;
  for (i = BMAX; i; i--)
    if (c[i])
      break;
  g = i;                        /* maximum code length */
  if ((unsigned)*m > i)
    *m = i;


  /* Adjust last length count to fill out codes, if needed */
  for (y = 1 << j; j < i; j++, y <<= 1)
    if ((y -= c[j]) < 0)
      return 2;                 /* bad input: more codes than bits */
  if ((y -= c[i]) < 0)
    return 2;
  c[i] += y;


  /* Generate starting offsets into the value table for each length */
  x[1] = j = 0;
  p = c + 1;  xp = x + 2;
  while (--i) {                 /* note that i == g from above */
    *xp++ = (j += *p++);
  }


  /* Make a table of values in order of bit lengths */
  memset(v, 0, sizeof v);
  p = (unsigned *)b;  i = 0;
  do {
    if ((j = *p++) != 0)
      v[x[j]++] = i;
  } while (++i < n);
  n = x[g];                     /* set n to length of v */


  /* Generate the Huffman codes and for each, make the table entries */
  x[0] = i = 0;                 /* first Huffman code is zero */
  p = v;                        /* grab values in bit order */
  h = -1;                       /* no tables yet--level -1 */
  w = l[-1] = 0;                /* no bits decoded yet */
  u[0] = NULL;   		/* just to keep compilers happy */
  q = NULL;     		 /* ditto */
  z = 0;                        /* ditto */

  /* go through the bit lengths (k already is bits in shortest code) */
  for (; k <= g; k++)
  {
    a = c[k];
    while (a--)
    {
      /* here i is the Huffman code of length k bits for value *p */
      /* make tables up to required level */
      while (k > w + l[h])
      {
        w += l[h++];            /* add bits already decoded */

        /* compute minimum size table less than or equal to *m bits */
        z = (z = g - w) > (unsigned)*m ? *m : z;        /* upper limit */
        if ((f = 1 << (j = k - w)) > a + 1)     /* try a k-w bit table */
        {                       /* too few codes for k-w bit table */
          f -= a + 1;           /* deduct codes from patterns left */
          xp = c + k;
          while (++j < z)       /* try smaller tables up to z bits */
          {
            if ((f <<= 1) <= *++xp)
              break;            /* enough codes to use up j bits */
            f -= *xp;           /* else deduct codes from patterns */
          }
        }
        if ((unsigned)w + j > el && (unsigned)w < el)
          j = el - w;           /* make EOB code end at table */
        z = 1 << j;             /* table entries for j-bit table */
        l[h] = j;               /* set table size in stack */

        /* allocate and link in new table */
        if ((q = malloc((z + 1)*sizeof(struct huft))) == NULL)
        {
          if (h)
            huft_free(u[0]);
          return 3;             /* not enough memory */
        }
#ifdef DEBUG
        G.hufts += z + 1;         /* track memory usage */
#endif
        *t = q + 1;             /* link to list for huft_free() */
        *(t = &(q->v.t)) = NULL;
        u[h] = ++q;             /* table starts after link */

        /* connect to last table, if there is one */
        if (h)
        {
          x[h] = i;             /* save pattern for backing up */
          r.b = (uint8_t)l[h-1];    /* bits to dump before this table */
          r.e = (uint8_t)(bits + j);  /* bits in this table */
          r.v.t = q;            /* pointer to this table */
          j = (i & ((1 << w) - 1)) >> (w - l[h-1]);
          u[h-1][j] = r;        /* connect to last table */
        }
      }

      /* set up table entry in r */
      r.b = (uint8_t)(k - w);
      if (p >= v + n)
        r.e = 99;               /* out of values--invalid code */
      else if (*p < s)
      {
        r.e = (uint8_t)(*p < 256 ? nob : eob);  /* 256 is end-of-block code */
        r.v.n = (uint16_t)*p++;           /* simple code is just the value */
      }
      else
      {
        r.e = (uint8_t)e[*p - s];   /* non-simple--look up in lists */
        r.v.n = d[*p++ - s];
      }

      /* fill code-like entries with r */
      f = 1 << (k - w);
      for (j = i >> w; j < z; j += f)
        q[j] = r;

      /* backwards increment the k-bit code i */
      for (j = 1 << (k - 1); i & j; j >>= 1)
        i ^= j;
      i ^= j;

      /* backup over finished tables */
      while ((i & ((1 << w) - 1)) != x[h])
        w -= l[--h];            /* don't need to update q */
    }
  }


  /* return actual size of base table */
  *m = l[0];


  /* Return true (1) if we were given an incomplete table */
  return y != 0 && g != 1;
}


void
huft_free(struct huft *t)
/*struct huft *t;*/         /* table to free */
/* Free the malloc'ed tables built by huft_build(), which makes a linked
   list of the tables it made, with the links in a dummy first entry of
   each table. */
{
  register struct huft *p, *q;


  /* Go through linked list, freeing from the malloced (t[-1]) address. */
  p = t;
  while (p != NULL)
  {
    q = (--p)->v.t;
    free(p);
    p = q;
  }
}

const uint16_t	mask_bits[] = {
	0x0000,
	0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x007f, 0x00ff,
	0x01ff, 0x03ff, 0x07ff, 0x0fff, 0x1fff, 0x3fff, 0x7fff, 0xffff
};

void
flush(struct globals *Gp, const void *data, size_t size)
{
	if (Gp->tfd>=0 && write(Gp->tfd, data, size) != size) {
		emsg(3, "Cannot write \"%s\"", Gp->tgt);
		Gp->tfd = -1;
		Gp->status = -1;
	}
	*Gp->crc = zipcrc(*Gp->crc, data, size);
}

int
readbyte(struct globals *Gp)
{
	if (Gp->uzsize <= 0)
		return EOF;
	Gp->incnt = bread((char *)Gp->inbuf,
		Gp->uzsize>sizeof Gp->inbuf?sizeof Gp->inbuf:Gp->uzsize);
	if (Gp->incnt <= 0)
		unexeoa();
	if (Gp->doswap)
		swap((char *)Gp->inbuf, Gp->incnt, bflag||sflag,bflag||Sflag);
	Gp->uzsize -= Gp->incnt;
	Gp->incnt--;
	Gp->inptr = Gp->inbuf;
	return (int)(*Gp->inptr++);
}
