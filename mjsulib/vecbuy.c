/* MJS Portable C Library.
 * Module: buy/grow a string-vector
 * @(#) vecbuy.c	1.0	28jan91 MJS
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
#include <stdlib.h>
#include "mjsu.h"
#include "mjsuimpl.h"


/* grow a string vector, optionally failing hard.
 * If v is NULL, a new vector is created on the heap; otherwise we
 * assumes that the vector at v is already allocated on the heap.
 * In either case we then append a copy of str to it.
 * Returns NULL if fails, else returns the (possibly new) vector.
 */
CHAR **vec_buy(CHAR **v, CHAR *str, BOOL force)
        {
        CHAR *s;
        UINT i;

        if (!str)
                return (v);

        if (!(s = str_dup(str, force)))
                return (NULL);

        if (!v) /* creating vector */
                {
                if (!(v = mem_buy(v, 1 * sizeof(*v), force)))
                        return (NULL);
                v[0] = NULL;
                }

        for (i = 0; v[i]; ++i)
                ;

        if (!(v = mem_buy(v, (i + 2) * sizeof(*v), force)))
                return (NULL);

        v[i++] = s;
        v[i] = NULL;
        return (v);
        }

