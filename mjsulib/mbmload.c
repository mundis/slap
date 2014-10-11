/* MJS Portable C Library.
 * Module: load bitmap from file
 * @(#) mbmload.c	1.0	30jun97 MJS
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mjsu.h"
#include "mjsuimpl.h"

MBITMAP *mbm_load(FILE *pf)
	{
	MBITMAP *bm;
	BYTE buf[8];
	LONG fpos;
	UINT n;
	USHORT h, w;

	fpos = ftell(pf);

	if (!fread(buf, 1, 8, pf))
		{
		fseek(pf, fpos, SEEK_SET);
		return (NULL);
		}
	n = strlen(MBM_MAGIC);
	if (strncmp((CHAR *)buf, MBM_MAGIC, n) ||
		((buf[n++] - 060) != MBM_VERSION) ||
		(buf[n++] != 015) ||
		(buf[n++] != 012))
		{
		fseek(pf, fpos, SEEK_SET);
		return (NULL);
		}

	if (!fread(&h, 1, sizeof(USHORT), pf) ||
		!fread(&w, 1, sizeof(USHORT), pf))
		{
		fseek(pf, fpos, SEEK_SET);
		return (NULL);
		}
	h = lstos(h);
	w = lstos(w);

	if (!(bm = mbm_buy(h, w, NO)))
		{
		fseek(pf, fpos, SEEK_SET);
		return (NULL);
		}

	if (fread(bm->bits, 1, mbm_need(h, w), pf) != mbm_need(h, w))
		{
		fseek(pf, fpos, SEEK_SET);
		mbm_free(bm);
		return (NULL);
		}

	return (bm);
	}
