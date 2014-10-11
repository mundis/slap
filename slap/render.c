/* slap: UNIX SmartLabel printing program, version 2
 * Module: routines to render text into a bitmap, and request
 *		display services from the (current) display driver.
 *
 * Copyright (c) 1994-1997, 2000 by Mike Spooner
 *----------------------------------------------------------------------
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
 *----------------------------------------------------------------------
 *
 * Core API:	ANSI X3.159-1989 C Language
 * Other APIs:	MJS Portable C Library
 *		MJS BitFont Library
 ***********************************************************************
 *
 * This renderer supports fonts in MBF format. Such format can be used to
 * express bitmapped fonts of any size, with metrics.
 *
 * Note that the font-format assumes a single-byte ASCII-based encoding
 * (eg: ISO 8859-1).
 *
 ******************************************************************************
 *
 * If "squeeze" is TRUE, leading blank-lines at the top of a label are removed.
 * Except where the font defines them to be glyphs, the common ASCII "control"
 * characters behave as you would expect:
 *
 *	'\n' (line-feed)	causes printing to continue at the start of
 *				the next line. If the next line would "fall
 *				off" the bottom of the page (label), printing
 *				continues at the start of the next page (label).
 *				NOTE: this means that a line-feed is actually
 *				interpreted as a notional "new-line", following
 *				the UNIX conventions for text-files.
 *				
 *	'\r' (carriage-return)	causes printing to continue at the start of
 *				the current line. Not especially useful, but
 *				standard practice.
 *
 *	'\b' (backspace)	causes printing to continue at the same
 *				place as the last character, thus providing
 *				an "overstrike" capability. Some UNIX programs
 *				expect to be able to do underlining via such
 *				backspacing behaviour. SPOOF: this is broken
 *				in that 
 *
 *	'\f' (form-feed)	causes printing to continue at the start of
 *				the next page (label).
 *
 *	'\t' (horiz. tab)	causes printing to continue at the next
 *				"tab-stop", which are set every 8 space-widths.
 *				If the font does not define a space or 'M' or
 *				'm' character, then the space-width is zero.
 *
 * Glyphs that would (even partially) "fall off" the right-hand end of the
 * output bitmap are silently discarded.
 * Lines that would "fall off" the bottom of the current output bitmap cause
 * the current bitmap page to be displayed, so that the offending line becomes
 * the first line of the next bitmap page.
 *
 ******************************************************************************
 *
 * The core rendering algorithm is simplex, and is thus not particularly
 * time-efficient. For slap, rendering and real-time printer-control are
 * completely decoupled, so this is not an issue.
 * 
 * This renderer does not deal with input data in "graphic" format, such as
 * PostScript, PCL, ditroff, dvi, plot(1), Sun raster, X pixmap, etc.
 *
 * The simplest way to support PostScript would be write a driver for
 * GNU GhostScript that produces SLP-format dotmaps. You would also need
 * to extract the printer-interface/protocol parts from this filter into a
 * standalone data-delivery program. It sounds like a lot of effort, but you
 * would be the first person in the world to have a Level 2 PostScript
 * label-printer!
 *
 ******************************************************************************
 */
static const char * const module = "@(#)" __FILE__ " 2.1 23jan2000 MJS";


#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "mjsu.h"
#include "slap.h"
#include "mjsfont.h"

#define max(a, b)	((a) > (b) ? (a) : (b))
#define min(a, b)	((a) < (b) ? (a) : (b))
#define TABSIZE	8


static VOID set_labelimage_bit(ARENA *a, UINT row, UINT col,
	UINT vheight, UINT vwidth)
	{
	assert((row < vheight) && (col < vwidth));

	if (mbm_height(a->bm) != vheight)
		{
		/* logical orientation differs from physical orientation */
		mbm_setb(a->bm, col, (vheight - 1) - row); 
		}
	else
		{
		/* logical and physical orientation are the same */
		mbm_setb(a->bm, row, col);
		}
	a->dirty = YES;
	return;
	}


static UINT put_glyph(ARENA *a, UINT row, UINT col, UINT vheight, UINT vwidth,
	FONT *font, GLYPH *g)
	{
	UINT i, j;
	INT extrawidth = 0;

	TRACE((9, "char @%u,%u: " \
		"left %d right %d ascent %d (%d) descent %d (%d) width %d",
		row, col,
		g->lbearing, g->rbearing,
		g->ascent, font->ascent,
		g->descent, font->descent,
		g->width));

	/* NOTE: row, col are the glyph "origin-point" (on the baseline) */

	if ((g->lbearing < 0) && (((UINT) -g->lbearing) > col))
		{
		col = 0;
		extrawidth = -g->lbearing;
		}
	else
		{
		col += g->lbearing;
		extrawidth = 0;
		}

	if (((col + g->rbearing) <= vwidth) &&
		(row >= g->ascent) &&
		((row + g->descent) <= vheight))
		{
		row -= g->ascent;
		for (i = 0; i < (g->ascent + g->descent); ++i)
			{
			for (j = 0; j < mbm_width(g->bm); ++j)
				{
				if (mbm_tstb(g->bm, i, j))
					{
					set_labelimage_bit(a, row+i, col+j,
						vheight, vwidth);
					}
				}
			}
		}
	return (col + g->width + extrawidth);
	}

/* draw an outline box round a bitmap
 */
static VOID boxit(MBITMAP *bm)
	{
	USHORT row, col;

	for (col = 0; col < mbm_width(bm); ++col)
		{
		mbm_setb(bm, 0, col); 
		mbm_setb(bm, mbm_height(bm) - 1, col); 
		}
	for (row = mbm_height(bm) - 1; row > 1; --row)
		{
		mbm_setb(bm, row, 0);
		mbm_setb(bm, row, mbm_width(bm) - 1);
		}
	}


/* render and print one copy of a file
 */
static VOID render1(SESSION *sd, FILE *pf, FONT *font, BOOL portrait,
	BOOL squeeze, BOOL boxed)
	{
	CHAR buf[BUFSIZ];
	UINT row, col;
	UINT vheight, vwidth;	/* "virtual" rather than physical - after
				 * normalising for orientation....
				 */
	CHAR *p, *q;
	UINT spacewidth;
	GLYPH *g;

	for (spacewidth = 0, p = " mM_"; !spacewidth && *p; ++p)
		spacewidth = mbm_width(font->glyphs[*p & 0xFF].bm);

	mbm_clr(sd->arena.bm);
	if (boxed)
		boxit(sd->arena.bm);
	sd->arena.dirty = NO;

	if (portrait)
		{
		vheight = max(mbm_width(sd->arena.bm),mbm_height(sd->arena.bm));
		vwidth = min(mbm_width(sd->arena.bm),mbm_height(sd->arena.bm));
		}
	else
		{
		vwidth = max(mbm_width(sd->arena.bm),mbm_height(sd->arena.bm));
		vheight = min(mbm_width(sd->arena.bm),mbm_height(sd->arena.bm));
		}

	TRACE((4, "HxW: physical=%ux%u, logical=%ux%u",
		mbm_height(sd->arena.bm), mbm_width(sd->arena.bm),
		vheight, vwidth));

	if ((font->ascent + font->descent) > vheight)
		Error("font is too tall for selected media-size or orientation");
	
	row = 0 + font->ascent;	/* position at font baseline! */
	col = 0;

	while (fgets(buf, BUFSIZ, pf))
		{
		/* first process embedded backspaces (so that backspacing
		 * over tabs and formfeeds comes out right!)...
		 */
		for (p = q = buf; *p; ++p)
			{
			if (*p == '\b')
				--q;
			else
				*q++ = *p;
			}
		*q = '\0';

		/* now render the buffer...
		 */
		for (p = buf; *p; ++p)
			{
			g = &font->glyphs[*p & 0xFF];

			if (g->width)
				{
				col = put_glyph(&sd->arena, row, col,
					vheight, vwidth, font, g);
				}
			else if (*p == '\t')
				{
				do
					{
					col += spacewidth;
					} while ((col / spacewidth) % TABSIZE);
				}
			else if (*p == '\n')
				{
				if (squeeze && !sd->arena.dirty)
					{
					/* then suppress leading blank line */
					continue;
					}
				row += font->descent + font->ascent;
				col = 0;	/* UNIX semantics! */

				/* if next line falls off the bottom of the label...
				 */
				if ((row + font->descent) >= vheight)
					{
					if (sd->arena.dirty)
						{
						/* then print the current label...
						 */
						sd->drv.print(sd->opd, &sd->arena);
						}

					/* and start a new label...
					 */
					mbm_clr(sd->arena.bm);
					if (boxed)
						boxit(sd->arena.bm);
					sd->arena.dirty = NO;
					row = 0 + font->ascent;
					col = 0;
					}
				}
			else if (*p == '\r')
				col = 0;
			else if (*p == '\f')	/* if we have a FORMFEED char... */
				{
				if (sd->arena.dirty)	/* and it is not redundant */
					{
					/* then print the current label...
					 */
					sd->drv.print(sd->opd, &sd->arena);
					}

				/* and start a new label...
				 */
				mbm_clr(sd->arena.bm);
				if (boxed)
					boxit(sd->arena.bm);
				sd->arena.dirty = NO;
				row = 0 + font->ascent;
				col = 0;
				}
			 else	/* a non-control-&-non-printing glyph: */
				col += spacewidth;
			}
		}

	if (sd->arena.dirty)		/* careful not to print blank pages! */
		sd->drv.print(sd->opd, &sd->arena);

	return;
	}


/* render and print requested number of copies of a (seekable) file...
 */
VOID render(SESSION *sd, FILE *pf, UINT numcopies, FONT *font,
	BOOL portrait, BOOL squeeze, BOOL boxed)
	{
	UINT i;

	for (i = 0; i < numcopies; ++i)
		{
		rewind(pf);
		render1(sd, pf, font, portrait, squeeze, boxed);
		}
	return;
	}
