/* MJS Portable C Library.
 * Module: notification-message API - vremark() and remark()
 * @(#) remark.c 1.4 25jan91
 *
 * Copyright (c) 1991 Mike Spooner
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
#include "mjsu.h"
#include "mjsuimpl.h"

static UINT nremarks = 0;

UINT vremark(CHAR *fmt, va_list ap)
	{
	if (fmt)
		{
		verrmsg(NULL, fmt, ap);
		++nremarks;
		}
	return (nremarks);
	}

UINT remark(CHAR *fmt, ...)
	{
	va_list ap;

	if (fmt)
		{
		va_start(ap, fmt);
		vremark(fmt, ap);
		va_end(ap);
		}

	return (nremarks);
	}
