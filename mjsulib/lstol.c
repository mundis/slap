/* MJS Portable C Library.
 * Module: convert 32-bit quantum from LSB-first into native byte-order
 * @(#) lstol.c  1.0 17sep95 MJS
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

/* convert 32-bit quantum from LSB-first into native byte-order
 */
ULONG lstol(ULONG src)
        {
        ULONG dest;
	BYTE *p = (BYTE *) &src;

	dest =  (ULONG) *p++ <<  0;
	dest |= (ULONG) *p++ <<  8;
	dest |= (ULONG) *p++ << 16;
	dest |= (ULONG) *p   << 24;
 
        return (dest);
        }
