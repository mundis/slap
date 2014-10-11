#ifndef __MJSFONT_H__
#define __MJSFONT_H__

#include "mjsu.h"
#include "mjsbm.h"

#define FFF_MAGIC	"mjsfff"	/* file-format identifier */
#define FFF_VERSION	2	/* file-format version */
#define FN_MAX		77	/* maximum length of a font name */
#define NGLYPHS		256	/* maximum number of glyphs in a font */


/* an in-core bitmap of a glyph
 */
typedef struct
	{
	UTINY lefthang;	/* left overhang, in pixels */
	UTINY righthang;/* right overhang, in pixels */
	MBM *bm;
	} GLYPH;


/* an in-core font:
 */
typedef struct
	{
	CHAR name[FN_MAX+1];	/* always NUL-terminated */
	UTINY height;		/* in pixels */
	UTINY dg;		/* index of default glyph */
	GLYPH glyphs[NGLYPHS];
	} FONT;
	


BOOL savefont(FONT *font, FILE *pf);
VOID dumpchar(FONT *f, BYTE idx);
FONT *loadfont(FILE *pf);
FONT *dropfont(FONT *f);

#endif
