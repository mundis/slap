/* MJS Portable C Library.
 * Module: clear specified bit of a bitmap
 * @(#) mbmclrb.c	1.0	30jun97 MJS
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

VOID mbm_clrb(MBITMAP *p, USHORT row, USHORT col)
	{
	UINT n = (row * p->width) + col;	/* linear bit-number */

	p->bits[n >> 3] &= ~(1 << (n & 7));
	}
