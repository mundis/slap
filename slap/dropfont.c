/* MJS BitFont library, version 1.
 * Module: deallocate an in-memory font
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
static const char * const module = "@(#)" __FILE__ " 1.0 29apr1997 MJS";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "mjsfont.h"


FONT *dropfont(FONT *f)
	{
	UINT n;

	for (n = 0; n < NGLYPHS; ++n)
		{
		if (f->glyphs[n].bm)
			mbm_free(f->glyphs[n].bm);
		}

	return (mem_free(f));
	}


