/* MJS BitFont library, version 1.
 * Module: load a font from a file into memory
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
static const char * const module = "@(#)" __FILE__ " 2.1 21jun1997 MJS";


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "mjsfont.h"


FONT *loadfont(FILE *pf)
	{
	ULONG fpos;
	UINT n;
	FONT *f = NULL;
	BYTE buf[BUFSIZ];

	fpos = ftell(pf);

	if (!fread(buf, 8, 1, pf))
		{
		fseek(pf, fpos, SEEK_SET);
		return (NULL);
		}
	n = strlen(FFF_MAGIC);
	if (strncmp((CHAR *)buf, FFF_MAGIC, n) ||
		((buf[n++] - 060) != FFF_VERSION) ||
		(buf[n++] != 015) ||
		(buf[n++] != 012))
		{
		fseek(pf, fpos, SEEK_SET);
		return (NULL);
		}

	f = mem_buy(f, sizeof(*f), YES);
	memset(&f->glyphs, 0, sizeof(f->glyphs));

	for (n = 0; n < 128; ++n)
		{
		if (!fread(&f->name[n], 1, 1, pf))
			{
			fseek(pf, fpos, SEEK_SET);
			return (mem_free(f));
			}
		if (f->name[n] == 012)
			break;
		}
	if (!n-- || (f->name[n] != 015))
		{
		fseek(pf, fpos, SEEK_SET);
		return (mem_free(f));
		}

	if (!fread(&f->ascent, sizeof(f->ascent), 1, pf) ||
		!fread(&f->descent, sizeof(f->descent), 1, pf))
		{
		fseek(pf, fpos, SEEK_SET);
		return (mem_free(f));
		}
	f->ascent = lstos(f->ascent);
	f->descent = lstos(f->descent);

	for (n = 0; n < NGLYPHS; ++n)
		{
		GLYPH *g = &f->glyphs[n];

		if (!fread(&g->ascent, sizeof(g->ascent), 1, pf) ||
			!fread(&g->descent, sizeof(g->descent), 1, pf) ||
			!fread(&g->lbearing, sizeof(g->lbearing), 1, pf) ||
			!fread(&g->rbearing, sizeof(g->rbearing), 1, pf) ||
			!fread(&g->width, sizeof(g->width), 1, pf))
			{
			return (dropfont(f));
			}
		g->ascent = lstos(g->ascent);
		g->descent = lstos(g->descent);
		g->lbearing = lstos(g->lbearing);
		g->rbearing = lstos(g->rbearing);
		g->width = lstos(g->width);
		if (!(g->bm = mbm_load(pf)))
			return (dropfont(f));
		}
	if (!checkfontmetrics(f))
		return (dropfont(f));

	return (f);
	}


