/* slap: UNIX SmartLabel printing program, version 2
 * Module: serial-port and open-file utility API header-file
 *
 * Copyright (c) 1997, 2001 by Mike Spooner
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
 * Core API:	ANSI X3.159-1989 C Language
 * Other APIs:	MJS Portable C Library
 *		IEEE Std 1003.1-1990 (aka POSIX.1)
 ****************************************************************************
 */
#ifndef __PORT_H__
#define __PORT_H__ 1

static const char * const port_h = "@(#)" __FILE__ " 1.1 30jan2001 MJS";

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include "mjsu.h"

/* the type representing a baud-rate mapping-scheme:
 */
typedef enum
	{
	BAUDMAP_NONE = 0,
	BAUDMAP_POSIX = 1,	/* actually the same scheme as NONE */
	BAUDMAP_AIX4_FAST=2,
	BAUDMAP_MAGMA_FAST=3
	} BAUD_MAPPING;


INT opentty(const CHAR *name, INT oflag, struct termios *save);
BOOL closetty(INT fd, const struct termios *restore);
BOOL isctty(INT fd);
BOOL fxlock(INT fd, INT timeout);
BOOL is_readable(INT fd);
BOOL is_writable(INT fd);
BOOL set_baud_mapping(const BAUD_MAPPING scheme);
INT baudcmp(speed_t, speed_t);
const CHAR *baud2name(speed_t);
speed_t val2baud(LONG val);
LONG baud2val(speed_t b);
BOOL faster_than(speed_t);
BOOL try_cts(INT fd);
BOOL use_cts(INT fd);
BOOL avoid_cts(INT fd);


/* map Irix-5.3 termios c_cflag RTS/CTS flow-control setting to the
 * common symbol-name:
 */
#if defined(CNEW_RTSCTS) && !defined(CRTSCTS)
#define CRTSCTS CNEW_RTSCTS
#endif

#endif
