/* MJS Portable C Library.
 * Module: compact simplex memory allocation
 * @(#) mem_buy.c  1.1 29dec89 MJS
 *
 * Copyright (c) 1989 Mike Spooner
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
#include "mjsu.h"


VOID *mem_buy(VOID *old, UINT size, BOOL force)
	{
	VOID *p;

	/* kludge-round NULL returns if size is zero!
	 */
	if (!size)
		++size;

	if (old)
		p = realloc(old, size);
	else
		p = malloc(size);
		

	if (!p && force)
		error("out of memory");
	return (p);
	}


VOID *mem_free(VOID *old)
	{
	if (old)
		free(old);
	return (NULL);
	}
