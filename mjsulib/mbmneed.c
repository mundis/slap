/* MJS Portable C Library.
 * Module: check representability of implied bitmap size
 * @(#) mbmneed.c	1.0	30jun97 MJS
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *----------------------------------------------------------------------
 * Source-code conforms to ANSI standard X3.159-1989.
 */
#include "mjsu.h"
#include "mjsuimpl.h"

/* check that the amount of storage needed for the bits of a bitmap
 * can be represented as a size_t value.
 * In 16-bit environments, size_t may be the same as USHORT, and bitmaps
 * geometry would thus be limited to the cases where:
 *
 *      mbm_need(height, width) <= maximum size_t value
 */
BOOL mbm_check_need(USHORT height, USHORT width)
	{
	ULONG h = height, w = width, vl;
	size_t v;

	vl = mbm_need(h, w);	/* NOTE: the macro evals this with LONGs! */
	v = (size_t) vl;
	return ((v == vl) ? YES : NO);
	}
