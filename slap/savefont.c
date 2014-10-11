/* MJS BitFont library, version 1.
 * Module: save an in-memory font to a file
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
static const char * const module = "@(#)" __FILE__ " 2.5 21jun1997 MJS";


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "mjsfont.h"


BOOL savefont(FONT *f, FILE *pf)
	{
	USHORT v;
	UINT n;

	if (!checkfontmetrics(f))	/* corrupt font */
		return (NO);

	if (fprintf(pf, "%s%c\r\n", FFF_MAGIC, FFF_VERSION + 060) < 0)
		return (NO);

	if (fputc(0, pf) == EOF)	/* empty flags! */
		return (NO);

	if (fprintf(pf, "%s\r\n", f->name) < 0)
		return (NO);

	v = stols(f->ascent);
	if (!fwrite(&v, sizeof(v), 1, pf))
		return (NO);

	v = stols(f->descent);
	if (!fwrite(&v, sizeof(v), 1, pf))
		return (NO);

	for (n = 0; n < NGLYPHS; ++n)
		{
		GLYPH *g = &f->glyphs[n];

		v = stols(g->ascent);
		if (!fwrite(&v, sizeof(v), 1, pf))
			return (NO);

		v = stols(g->descent);
		if (!fwrite(&v, sizeof(v), 1, pf))
			return (NO);

		v = stols(g->lbearing);
		if (!fwrite(&v, sizeof(v), 1, pf))
			return (NO);

		v = stols(g->rbearing);
		if (!fwrite(&v, sizeof(v), 1, pf))
			return (NO);

		v = stols(g->width);
		if (!fwrite(&v, sizeof(v), 1, pf))
			return (NO);

		if (!mbm_save(g->bm, pf))
			return (NO);
		}
	return (YES);
	}

