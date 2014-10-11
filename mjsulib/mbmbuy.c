/* MJS Portable C Library.
 * Module: allocate blank in-memory bitmap
 * @(#) mbmbuy.c	1.0	30jun97 MJS
 *
 * Copyright (c) 1997 Mike Spooner
 *----------------------------------------------------------------------
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *----------------------------------------------------------------------
 * Source-code conforms to ANSI standard X3.159-1989.
 */
#include <stdlib.h>
#include "mjsu.h"
#include "mjsuimpl.h"

MBITMAP *mbm_buy(USHORT height, USHORT width, BOOL force)
	{
	MBITMAP *p = NULL;

	if (!(p = mem_buy(p, sizeof(*p), force)))
		return (NULL);
	p->bits = NULL;
	p->height = height;
	p->width = width;

	if (!mbm_check_need(height, width) ||
		!(p->bits = mem_buy(p->bits, mbm_need(height, width), force)))
		{
		free(p);
		return (NULL);
		}

	mbm_clr(p);
	return (p);
	}
