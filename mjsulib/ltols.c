/* MJS Portable C Library.
 * Module: convert 32-bit quantum from native to LSB-first byte-order
 * @(#) ltols.c  1.0 17sep95 MJS
 *
 * Copyright (c) 1995 Mike Spooner
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


/* convert 32-bit quantum from native to LSB-first byte-order
 */
ULONG ltols(ULONG src)
	{
	ULONG dest;
	BYTE *p = (BYTE *) &dest;

	*p++ = (src >>  0) & 0xFF;
	*p++ = (src >>  8) & 0xFF;
	*p++ = (src >> 16) & 0xFF;
	*p   = (src >> 24) & 0xFF;

	return (dest);
	}
