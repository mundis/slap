/* MJS Portable C Library.
 * Module: flexible string-duplication
 * @(#) str_dup.c	1.0	05may92 MJS
 *
 * Copyright (c) 1992 Mike Spooner
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
#include <stdlib.h>
#include <string.h>
#include "mjsu.h"


/* duplicate a string onto the heap.
 */
CHAR *str_dup(const CHAR *src, BOOL force)
	{
	CHAR *p;

	p = malloc(strlen(src) + 1);
	if (p)
		strcpy(p, src);
	else if (force)
		error("out of memory");
	return (p);
	}

