/* MJS Portable C Library.
 * Module: internal private inter-module declarations
 *
 * Copyright (c) 1989-1997, 2005 Mike Spooner
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
 *----------------------------------------------------------------------
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include "mjsu.h"

/* shared private RTW-hash map table: */
#define hashmap mjphsm
extern const BYTE hashmap[256];


/* shared default error-/warning-message-output function: */
extern VOID mjs_verrmsg(CHAR *msg_class, CHAR *fmt, va_list ap);


/* the name by which the current process was invoked, set by getflags(),
 * used as intro to error- and warning-messages:
 */
#define process_name mjpprn
extern CHAR *process_name;


#define min(a, b) (((a) < (b)) ? (a) : (b))
#define OCTET_MAX       255U    /* sic */

#define mbm_check_need mjbmck
BOOL mbm_check_need(USHORT height, USHORT width);

/* compute amount of memory needed for the bits of an MBM-format bitmap, without
 * checking for arithmetic overflow when expressed as a size_t.
 */
#define mbm_need(h, w)          ((((h) * (w)) + 7U) >> 3)

/* refer to library version string, to force it to be included in any users
 * of the library.
 */
#define mjsulibver mjlbvr
extern const CHAR * const mjsulibver;
static const CHAR * const * const volatile mjsu_verstr = &mjsulibver;

#ifdef __cplusplus
}
#endif
