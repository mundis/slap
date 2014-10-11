/* MJS BitFont library, version 1.
 * Module: validate an in-memory font structure
 *
 * Copyright (c) 1997 by Mike Spooner
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
static const char * const module = "@(#)" __FILE__ " 1.1 30jun1997 MJS";


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "mjsfont.h"


BOOL checkfontmetrics(FONT *f)
	{
	GLYPH *g;
	UINT n;

	if ((f->ascent + f->descent) < 0)
		return (NO);
	
	for (g = f->glyphs, n = 0; n < NGLYPHS; ++n, ++g)
		{
		if (g->ascent > f->ascent)
			return (NO);
		if (g->descent > f->descent)
			return (NO);
		if ((g->ascent + g->descent) < 0)
			return (NO);
		if (g->lbearing > g->rbearing)
			return (NO);
		if (((USHORT) (g->ascent + g->descent)) != g->bm->height)
			return (NO);
		if (((USHORT) (g->rbearing - g->lbearing)) != g->bm->width)
			return (NO);
		}

	return (YES);
	}


