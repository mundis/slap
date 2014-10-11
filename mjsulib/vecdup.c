/* MJS Portable C Library.
 * Module: duplicate a string-vector
 * @(#) vecdup.c	1.0	28jan91 MJS
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

/* duplicate a string vector.
 * Storage for the duplicate is allocated from the heap.
 * Returns NULL on failure, else returns the freshly-minted vector.
 */
CHAR **vec_dup(CHAR **v, BOOL force)
        {
        CHAR **new = NULL;
        UINT i;

        if (!v)
                return (NULL);

        for (i = 0; v[i]; ++i)
                ;
        if (!(new = mem_buy(new, (i + 1) * sizeof(*v), force)))
                return (NULL);

        for (i = 0; v[i]; ++i)
                {
                new[i] = str_dup(v[i], force);
                if (!new[i])    /* could not allocate an element */
                        {
                        UINT j;
                        /* must now free up previous stuff and return NULL */
                        for (j = 0; j < i; ++j)
                                free(new[j]);
                        free(new);
                        return (NULL);
                        }
                }
        new[i] = NULL;

        return (new);
	}
