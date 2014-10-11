/* MJS BitFont library, version 1.
 * Module: dump an ASCII representation of an in-memory font
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
static const char * const module = "@(#)" __FILE__ " 1.2 30jun1998 MJS";


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "mjsfont.h"


VOID dumpchar(FONT *f, BYTE idx)
	{
	MBITMAP *p = f->glyphs[idx].bm;
	UINT i, j, bit;

	if (p && mbm_height(p) && mbm_width(p))
		{
		bit = 0;
		for (i = 0; i < mbm_height(p); ++i)
			{
			for (j = 0; j < mbm_width(p); ++j, ++bit)
				{
				if (mbm_tstb(p, i, j))
					printf("#");
				else
					printf("-");
				}
			printf("\n");
			fflush(stdout);
			}
		}
	}

