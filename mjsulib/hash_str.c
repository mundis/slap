/* MJS Portable C Library.
 * Module: trivial string-hash function.
 * @(#)hash_str.c	1.0	10mar2005 MJS
 *
 * Copyright (c) 2005 Mike Spooner
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
#include <string.h>
#include <limits.h>
#include "mjsu.h"

/* compute a 16-bit hash value for a string using a random-table walk.
 * A wonderfully fast hashing algorithm with highly distributed outputs.
 */
USHORT strhash(const CHAR *key)
	{
	return (rtw_short_hash((BYTE *) key, strlen(key)));
	}
