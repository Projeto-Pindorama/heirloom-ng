/*
 * Changes by Gunnar Ritter, Freiburg i. Br., Germany, May 2003.
 *
 * Derived from unzip 5.40.
 *
 * Sccsid @(#)unzip.h	1.5 (gritter) 7/16/04
 */

#include <inttypes.h>

#define	Trace(a)

#define	MAX_BITS	13
#define	HSIZE		(1 << MAX_BITS)
#define	WSIZE		65536L	/* at least 64K for enhanced deflate */

#define	redirSlide	G.area.Slide

#define	NEXTBYTE	(--G.incnt >= 0 ? (int)(*G.inptr++) : readbyte(&G))

#define	READBITS(nbits, zdest)	{ \
	if (nbits > G.bits_left) { \
		int temp; \
		G.zipeof = 1; \
		while (G.bits_left <= 8 * (int)(sizeof G.bitbuf - 1) && \
				(temp = NEXTBYTE) != EOF) { \
			G.bitbuf |= (uint32_t)temp << G.bits_left; \
			G.bits_left += 8; \
			G.zipeof = 0; \
		} \
	} \
	zdest = (int16_t)((uint16_t)G.bitbuf & mask_bits[nbits]); \
	G.bitbuf >>= nbits; \
	G.bits_left -= nbits; \
}

#undef	FALSE
#undef	TRUE
enum	{
	FALSE = 0,
	TRUE = 1
};

union	work {
	struct	{
		int16_t	Parent[HSIZE];
		uint8_t	value[HSIZE];
		uint8_t	Stack[HSIZE];
	} shrink;
	uint8_t	Slide[WSIZE];
};

#define	OUTBUFSIZ	4096

struct	globals {
	union work	area;
	uint8_t	inbuf[4096];
	long long	zsize;
	long long	uzsize;
	uint8_t	*inptr;
	const char	*tgt;
	struct huft	*fixed_tl;
	struct huft	*fixed_td;
	struct huft	*fixed_tl64;
	struct huft	*fixed_td64;
	struct huft	*fixed_tl32;
	struct huft	*fixed_td32;
	const uint16_t	*cplens;
	const uint8_t	*cplext;
	const uint8_t	*cpdext;
	long	csize;
	long	ucsize;
	uint8_t	outbuf[OUTBUFSIZ];
	uint8_t	*outptr;
	uint32_t	*crc;
	uint32_t	bitbuf;
	uint32_t	wp;
	uint32_t	bb;
	uint32_t	bk;
	unsigned	outcnt;
	int	tfd;
	int	doswap;
	int	incnt;
	int	bits_left;
	int	zipeof;
	int	fixed_bl;
	int	fixed_bd;
	int	fixed_bl64;
	int	fixed_bd64;
	int	fixed_bl32;
	int	fixed_bd32;
	int	status;
};

/* Huffman code lookup table entry--this entry is four bytes for machines
   that have 16-bit pointers (e.g. PC's in the small or medium model).
   Valid extra bits are 0..13.  e == 15 is EOB (end of block), e == 16
   means that v is a literal, 16 < e < 32 means that v is a pointer to
   the next table, which codes e - 16 bits, and lastly e == 99 indicates
   an unused code.  If a code with e == 99 is looked up, this implies an
   error in the data. */

struct huft {
    uint8_t e;            /* number of extra bits or operation */
    uint8_t b;            /* number of bits in this code or subcode */
    union {
        uint16_t n;       /* literal, length base, or distance base */
        struct huft *t;   /* pointer to next level of table */
    } v;
};

extern const uint16_t	mask_bits[];

extern void	flush(struct globals *, const void *, size_t);
extern int	readbyte(struct globals *);

extern int	huft_build(const unsigned *b, unsigned n, unsigned s,
			const uint16_t *d, const uint8_t *e,
			struct huft **t, int *m,
			int bits, int nob, int eob);
extern void	huft_free(struct huft *);
