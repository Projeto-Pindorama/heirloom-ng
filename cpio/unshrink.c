/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, May 2003.
 *
 * Derived from unzip 5.40.
 *
 * Sccsid @(#)unshrink.c	1.4 (gritter) 6/18/04
 */
/*---------------------------------------------------------------------------

  unshrink.c                     version 1.21                     23 Nov 95


       NOTE:  This code may or may not infringe on the so-called "Welch
       patent" owned by Unisys.  (From reading the patent, it appears
       that a pure LZW decompressor is *not* covered, but this claim has
       not been tested in court, and Unisys is reported to believe other-
       wise.)  It is therefore the responsibility of the user to acquire
       whatever license(s) may be required for legal use of this code.

       THE INFO-ZIP GROUP DISCLAIMS ALL LIABILITY FOR USE OF THIS CODE
       IN VIOLATION OF APPLICABLE PATENT LAW.


  Shrinking is basically a dynamic LZW algorithm with allowed code sizes of
  up to 13 bits; in addition, there is provision for partial clearing of
  leaf nodes.  PKWARE uses the special code 256 (decimal) to indicate a
  change in code size or a partial clear of the code tree:  256,1 for the
  former and 256,2 for the latter.  [Note that partial clearing can "orphan"
  nodes:  the parent-to-be can be cleared before its new child is added,
  but the child is added anyway (as an orphan, as though the parent still
  existed).  When the tree fills up to the point where the parent node is
  reused, the orphan is effectively "adopted."  Versions prior to 1.05 were
  affected more due to greater use of pointers (to children and siblings
  as well as parents).]

  This replacement version of unshrink.c was written from scratch.  It is
  based only on the algorithms described in Mark Nelson's _The Data Compres-
  sion Book_ and in Terry Welch's original paper in the June 1984 issue of
  IEEE _Computer_; no existing source code, including any in Nelson's book,
  was used.

  Memory requirements have been reduced in this version and are now no more
  than the original Sam Smith code.  This is still larger than any of the
  other algorithms:  at a minimum, 8K+8K+16K (stack+values+parents) assuming
  16-bit short ints, and this does not even include the output buffer (the
  other algorithms leave the uncompressed data in the work area, typically
  called slide[]).  For machines with a 64KB data space this is a problem,
  particularly when text conversion is required and line endings have more
  than one character.  UnZip's solution is to use two roughly equal halves
  of outbuf for the ASCII conversion in such a case; the "unshrink" argument
  to flush() signals that this is the case.

  For large-memory machines, a second outbuf is allocated for translations,
  but only if unshrinking and only if translations are required.

              | binary mode  |        text mode
    ---------------------------------------------------
    big mem   |  big outbuf  | big outbuf + big outbuf2  <- malloc'd here
    small mem | small outbuf | half + half small outbuf

  Copyright 1994, 1995 Greg Roelofs.  See the accompanying file "COPYING"
  in UnZip 5.20 (or later) source or binary distributions.

  From "COPYING":

   The following copyright applies to the new version of unshrink.c,
   distributed with UnZip version 5.2 and later:

     * Copyright (c) 1994 Greg Roelofs.
     * Permission is granted to any individual/institution/corporate
     * entity to use, copy, redistribute or modify this software for
     * any purpose whatsoever, subject to the conditions noted in the
     * Frequently Asked Questions section below, plus one additional
     * condition:  namely, that my name not be removed from the source
     * code.  (Other names may, of course, be added as modifications
     * are made.)  Corporate legal staff (like at IBM :-) ) who have
     * problems understanding this can contact me through Zip-Bugs...


   Q. Can I use the source code of Zip and UnZip in my commercial
      application?

   A. Yes, so long as you include in your product an acknowledgment; a
      pointer to the original, free compression sources; and a statement
      making it clear that there are no extra or hidden charges resulting
      from the use of our compression code in your product (see below for
      an example).  The acknowledgment should appear in at least one piece
      of human-readable documentation (e.g., a README file or man page),
      although additionally putting it in the executable(s) is OK, too.
      In other words, you are allowed to sell only your own work, not ours,
      and we'd like a little credit.  (Note the additional restrictions
      above on the code in unreduce.c, unshrink.c, vms.c, time_lib.c, and
      everything in the wince and windll subdirectories.)  Contact us at
      Zip-Bugs@lists.wku.edu if you have special requirements.  We also
      like to hear when our code is being used, but we don't require that.

         <Product> incorporates compression code from the Info-ZIP group.
         There are no extra charges or costs due to the use of this code,
         and the original compression sources are freely available from
         http://www.cdrom.com/pub/infozip/ or ftp://ftp.cdrom.com/pub/infozip/
         on the Internet.

      If you only need compression capability, not full zipfile support,
      you might want to look at zlib instead; it has fewer restrictions
      on commercial use.  See http://www.cdrom.com/pub/infozip/zlib/ .

  ---------------------------------------------------------------------------*/

#include <string.h>
#include <stdio.h>

#include "cpio.h"
#include "unzip.h"

static void	partial_clear(struct globals *);

#define	trace()

/* HSIZE is defined as 2^13 (8192) in unzip.h */
#define	BOGUSCODE  256
#define	FLAG_BITS  parent        /* upper bits of parent[] used as flag bits */
#define	CODE_MASK  (HSIZE - 1)   /* 0x1fff (lower bits are parent's index) */
#define	FREE_CODE  HSIZE         /* 0x2000 (code is unused or was cleared) */
#define	HAS_CHILD  (HSIZE << 1)  /* 0x4000 (code has a child--do not clear) */

#define parent G.area.shrink.Parent
#define Value  G.area.shrink.value /* "value" conflicts with Pyramid ioctl.h */
#define stack  G.area.shrink.Stack

/***********************/
/* Function unshrink() */
/***********************/

int
zipunshrink(struct file *f, const char *tgt, int tfd, int doswap, uint32_t *crc)
{
    struct globals G;
    int offset = (HSIZE - 1);
    uint8_t *stacktop = stack + offset;
    register uint8_t *newstr;
    int codesize=9, len, KwKwK;
    int16_t code, oldcode, freecode, curcode;
    int16_t lastfreecode;
    unsigned int outbufsiz;

/*---------------------------------------------------------------------------
    Initialize various variables.
  ---------------------------------------------------------------------------*/

    memset(&G, 0, sizeof G);
    G.tgt = tgt;
    G.tfd = tfd;
    G.doswap = doswap;
    G.crc = crc;
    G.zsize = G.uzsize = f->f_csize;
    lastfreecode = BOGUSCODE;

    for (code = 0;  code < BOGUSCODE;  ++code) {
        Value[code] = (uint8_t)code;
        parent[code] = BOGUSCODE;
    }
    for (code = BOGUSCODE+1;  code < HSIZE;  ++code)
        parent[code] = FREE_CODE;

    outbufsiz = OUTBUFSIZ;
    G.outptr = G.outbuf;
    G.outcnt = 0L;

/*---------------------------------------------------------------------------
    Get and output first code, then loop over remaining ones.
  ---------------------------------------------------------------------------*/

    READBITS(codesize, oldcode)
    if (!G.zipeof) {
        *G.outptr++ = (uint8_t)oldcode;
        ++G.outcnt;
    }

    do {
        READBITS(codesize, code)
        if (G.zipeof)
            break;
        if (code == BOGUSCODE) {   /* possible to have consecutive escapes? */
            READBITS(codesize, code)
            if (code == 1) {
                ++codesize;
                Trace((stderr, " (codesize now %d bits)\n", codesize));
            } else if (code == 2) {
                Trace((stderr, " (partial clear code)\n"));
                partial_clear(&G);   /* clear leafs (nodes with no children) */
                Trace((stderr, " (done with partial clear)\n"));
                lastfreecode = BOGUSCODE;  /* reset start of free-node search */
            }
            continue;
        }

    /*-----------------------------------------------------------------------
        Translate code:  traverse tree from leaf back to root.
      -----------------------------------------------------------------------*/

        newstr = stacktop;
        curcode = code;

        if (parent[curcode] == FREE_CODE) {
            /* or (FLAG_BITS[curcode] & FREE_CODE)? */
            KwKwK = TRUE;
            Trace((stderr, " (found a KwKwK code %d; oldcode = %d)\n", code,
              oldcode));
            --newstr;   /* last character will be same as first character */
            curcode = oldcode;
        } else
            KwKwK = FALSE;

        do {
            *newstr-- = Value[curcode];
            curcode = (int16_t)(parent[curcode] & CODE_MASK);
        } while (curcode != BOGUSCODE);

        len = (int)(stacktop - newstr++);
        if (KwKwK)
            *stacktop = *newstr;

    /*-----------------------------------------------------------------------
        Write expanded string in reverse order to output buffer.
      -----------------------------------------------------------------------*/

        Trace((stderr, "code %4d; oldcode %4d; char %3d (%c); string [", code,
          oldcode, (int)(*newstr), (*newstr<32 || *newstr>=127)? ' ':*newstr));

        {
            register uint8_t *p;

            for (p = newstr;  p < newstr+len;  ++p) {
                *G.outptr++ = *p;
                if (++G.outcnt == outbufsiz) {
		    flush(&G, G.outbuf, G.outcnt);
                    G.outptr = G.outbuf;
                    G.outcnt = 0L;
                }
            }
        }

    /*-----------------------------------------------------------------------
        Add new leaf (first character of newstr) to tree as child of oldcode.
      -----------------------------------------------------------------------*/

        /* search for freecode */
        freecode = (int16_t)(lastfreecode + 1);
        /* add if-test before loop for speed? */
        while (parent[freecode] != FREE_CODE)
            ++freecode;
        lastfreecode = freecode;
        Trace((stderr, "]; newcode %d\n", freecode));

        Value[freecode] = *newstr;
        parent[freecode] = oldcode;
        oldcode = code;

    } while (!G.zipeof);

/*---------------------------------------------------------------------------
    Flush any remaining data and return to sender...
  ---------------------------------------------------------------------------*/

    if (G.outcnt > 0L)
	    flush(&G, G.outbuf, G.outcnt);

    return G.status;

} /* end function unshrink() */





/****************************/
/* Function partial_clear() */      /* no longer recursive... */
/****************************/

static void
partial_clear(struct globals *Gp)
{
#define	G	(*Gp)
    register int16_t code;

    /* clear all nodes which have no children (i.e., leaf nodes only) */

    /* first loop:  mark each parent as such */
    for (code = BOGUSCODE+1;  code < HSIZE;  ++code) {
        register int16_t cparent = (int16_t)(parent[code] & CODE_MASK);

        if (cparent > BOGUSCODE && cparent != FREE_CODE)
            FLAG_BITS[cparent] |= HAS_CHILD;   /* set parent's child-bit */
    }

    /* second loop:  clear all nodes *not* marked as parents; reset flag bits */
    for (code = BOGUSCODE+1;  code < HSIZE;  ++code) {
        if (FLAG_BITS[code] & HAS_CHILD)    /* just clear child-bit */
            FLAG_BITS[code] &= ~HAS_CHILD;
        else {                              /* leaf:  lose it */
            Trace((stderr, "%d\n", code));
            parent[code] = FREE_CODE;
        }
    }

    return;
}
