/* slap: UNIX SmartLabel printing program, version 2
 * Module: report program identity, version and configuration.
 *
 * Copyright (c) 1999, 2001 by Mike Spooner
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
 * Core API:    ANSI X3.159-1989 C Language
 * Other APIs:  MJS Portable C Library
 *              IEEE Std 1003.1-1990 (aka POSIX.1)
 ***********************************************************************
 */
static const char * const module = "@(#)" __FILE__ " 2.4.4  09feb2001 MJS";
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <signal.h>
#include "mjsu.h"
#include "port.h"


static const char * const version = "@(#)slap   2.4.4   09feb2001 MJS";
static const char * const cright = "@(#)Copyright (c) 1997-2001 by Mike Spooner";

void identity(void)
	{
	#ifndef DEBUG

	fprintf(stderr, "\n%s\n\n%s\n%s\n\n",
		version+4,
		"text-printing program for Seiko Smart Label Printer family",
		cright+4);

	fprintf(stderr, "Configured Options:\n");

	#else

		#ifdef __STDC__
			printf("__STDC__ = %lu\n",
				(unsigned long) __STDC__);
		#else
			printf("__STDC__ = undefined\n");
		#endif

		#ifdef _POSIX_SOURCE
			printf("_POSIX_SOURCE = %lu\n",
				(unsigned long) _POSIX_SOURCE);
		#else
			printf("_POSIX_SOURCE = undefined\n");
		#endif

		#ifdef _POSIX_C_SOURCE
			printf("_POSIX_C_SOURCE = %lu\n",
				(unsigned long) _POSIX_C_SOURCE);
		#else
			printf("_POSIX_C_SOURCE = undefined\n");
		#endif

		#ifdef _XOPEN_SOURCE
			printf("_XOPEN_SOURCE = %lu\n",
				(unsigned long) _XOPEN_SOURCE);
		#else
			printf("_XOPEN_SOURCE = undefined\n");
		#endif

		printf("__EXTENSIONS__ is %s\n",
			#ifdef __EXTENSIONS__
				"defined"
			#else
				"undefined"
			#endif
			);

	#endif

	fprintf(stderr, "\tbaud rates above 38400 %s available without mapping\n",
		#ifdef B57600
			"are"
		#else
			"are not"
		#endif
		);

	fprintf(stderr, "\tRTS/CTS flow-control is %s available\n",
		#ifdef CRTSCTS
			"is"
		#else
			"is not"
		#endif
		);

	fprintf(stderr, "\ttcdrain() %s\n",
		#if (TCDRAIN_BROKEN != 0)
			"returns early"
		#else
			"works properly"
		#endif
		);
	}

#ifdef DEBUG
int main(void)
	{
	identity();
	return (EXIT_SUCCESS);
	}
#endif
