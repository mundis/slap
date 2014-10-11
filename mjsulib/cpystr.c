/* MJS Portable C Library.
 * Module: Copy Multiple Strings
 * @(#) cpystr.c	1.0	12jun89 MJS
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
#include <stdarg.h>
#include <stddef.h>
#include "mjsu.h"




CHAR *cpystr(CHAR *dest, ...)
	{
	CHAR *q;
	va_list ap;

	va_start(ap, dest);
	while ((q = va_arg(ap, char *)) != NULL)
		{
		while (*q)
			*dest++ = *q++;
		}
	va_end(ap);
	*dest = '\0';
	return (dest);
	}
