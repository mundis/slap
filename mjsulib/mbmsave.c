/* MJS Portable C Library.
 * Module: save bitmap to file
 * @(#) mbmsave.c	1.0	30jun97 MJS
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

BOOL mbm_save(MBITMAP *bm, FILE *pf)
        {
        BYTE buf[8];
        UINT n;
        LONG fpos;
        USHORT h, w;

        fpos = ftell(pf);

        n = strlen(MBM_MAGIC);
        strncpy((CHAR *)buf, MBM_MAGIC, n);
        buf[n++] = 060 + MBM_VERSION;
        buf[n++] = 015;
        buf[n++] = 012;
        buf[n++] = 0;
        h = stols(bm->height);
        w = stols(bm->width);

        if (!mbm_check_need(bm->height, bm->width) || /* fwrite uses size_t! */
                !fwrite(buf, 1, n, pf) ||
                !fwrite(&h, 1, sizeof(h), pf) ||
                !fwrite(&w, 1, sizeof(w), pf) ||
                (fwrite(bm->bits, 1, mbm_need(bm->height, bm->width), pf) !=
                        mbm_need(bm->height, bm->width)))
                {
                fseek(pf, fpos, SEEK_SET);
                return (NO);
                }

        return (YES);
        }

