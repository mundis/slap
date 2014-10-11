/* MJS BitFont library, version 1.
 * Module: API header-file
 *
 * Copyright (c) 1997-1998 by Mike Spooner
 *------------------------------------------------------------------------
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *-----------------------------------------------------------------------
 */
#ifndef __MJSFONT_H__
#define __MJSFONT_H__

static const char * const mjsfont_h = "@(#)" __FILE__ " 2.0 28jun1997 MJS";

#include "mjsu.h"

#define FFF_MAGIC	"<MBF"	/* file-format identifier */
#define FFF_VERSION	2	/* file-format version */
#define FN_MAX		128	/* maximum length of a font name */
#define NGLYPHS		256	/* maximum number of glyphs in a font */


/* an in-core bitmap of a glyph...
 * The "metric" fields are named and interpreted as per X11r5 XCharStruct;
 * except that ascent and descent MUST be no greater than the values for the
 * font as a whole.
 * Note that ascent, descent, lbearing and rbearing can be negative.
 * Undefined characters are represented as glyphs with zero metrics, but still
 * have a (zero-height/zero-width) bitmap allocated.
 */
typedef struct
	{
	SHORT ascent, descent;		/* in pixels */
	SHORT lbearing, rbearing;	/* in pixels */
	SHORT pad[17];
	SHORT width;			/* of bounding-box, in pixels */
	MBITMAP *bm;			/* location within the bounding-box 
					 * is specified by the metrics */
	} GLYPH;


/* an in-core font...
 * The "metric" fields are as named and interpreted as per X11r5 XFontStruct.
 * Note that ascent and descent may be negative.
 */
typedef struct
	{
	CHAR name[FN_MAX+1];	/* always NUL-terminated */
	SHORT ascent, descent;	/* in pixels */
	USHORT dg;		/* index of default glyph */
	GLYPH glyphs[NGLYPHS];
	} FONT;
	


BOOL savefont(FONT *font, FILE *pf);
VOID dumpchar(FONT *f, BYTE idx);
FONT *loadfont(FILE *pf);
FONT *dropfont(FONT *f);
BOOL checkfontmetrics(FONT *f);

#endif
