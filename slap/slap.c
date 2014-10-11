/* slap: UNIX SmartLabel printing program, version 2
 * Module: command-line interface, font-locator, job dispatcher,
 *		and some utility functions
 *
 * Copyright (c) 1994-1998, 2000, 2001 by Mike Spooner
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
 *
 * Core API:	ANSI X3.159-1989 C Language
 * Other APIs:	MJS Portable C Library
 *		MJS BitFont Library
 *		IEEE Std 1003.1-1990 (aka POSIX.1)
 ***********************************************************************
 *
 * "slap" is a major rewrite of an earlier incarnation ("slpd",
 * late 1994) which only supported the SmartLabel-Plus.
 * The new "slap" differs from the old "slpd" thus:
 *
 *	Supports the SLP-Pro, SLP-EZ30, SLP-100, SLP-120, SLP-200, SLP-220
 *	and SLP-240, as well as the (obsolete) original SLP and SLP-Plus
 *	printers.
 *
 *	Modular driver-based architecture to allow eg: other printers,
 *	WYSYWIG X-Window display, etc.
 *
 *	New more flexible font architecture, and a completely new set of
 *	bitmapped fonts (now in several sizes! Gosh!).
 *
 *	SLP, SLP-Plus (and SLP-EZ30) printers run in interleaved "high-res"
 *	mode.
 *
 *	Much faster printing to the SLP-Plus (twice as fast, even
 *	with the increased resolution).
 *
 *	Lower system-CPU overhead.
 *
 *	Proper "out-of-paper" handling - waits for labels to be loaded,
 *	then resumes where it left off.
 *
 *	Proper "interrupt" handling - shuts down the printer and comms
 *	port cleanly (and non-recursively!).
 *
 *	Backspacing over a TAB character now works!
 *
 * In spite of all this, the old "slpd" served fine duty for nearly
 * three years, and was a MASSIVE improvement over previous
 * label-printing schemes:
 *
 *	(1) laser-printed sheets - very expensive, and awfully wasteful
 *		when printing labels one-at-a-time.
 *	(2) dotmatrix-printed labels - incredibly unreliable (the damn
 *		printers were always misfeeding and jamming) and very
 *		noisy (the requirement was for "close-to-hand" labels).
 *
 ***********************************************************************
 *
 * Thanks to Seiko Intruments Inc. for producing some really neat,
 * quiet, cheap and reliable label-printers.
 *
 ***********************************************************************
 */
static const char * const module = "@(#)" __FILE__ " 1.4 30jan2001 MJS";

#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include "mjsu.h"
#include "mjsfont.h"
#include "slap.h"
#include "port.h"

#if DEBUG_LEVEL
/* emit trace message to STDERR
 */
VOID trace(UINT level, CHAR *fmt, ...)
	{

	if (level <= DEBUG_LEVEL)
		{
		va_list ap;

		fprintf(stderr, "trace: ");
		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
		fprintf(stderr, "\n");
		}
	return;
	}
#endif


/* a kludge to get round the fact that POSIX.1 allows implementations
 * to omit a definition of PATH_MAX. This doesn't avoid the need to
 * dynamically allocate the storage, but it does avoid making LOTS of
 * calls to pathconf()...
 */
#ifndef PATH_MAX
LONG PATH_MAX = _POSIX_PATH_MAX;	/* overridden at init time */

VOID set_path_max(VOID)
	{

	if ((PATH_MAX = pathconf("/", _PC_PATH_MAX)) < _POSIX_PATH_MAX)
		PATH_MAX = _POSIX_PATH_MAX;
	}
#endif


static FONT *try_load_font(const CHAR *path)
	{
	FILE *pf = NULL;
	FONT *f = NULL;

	if ((pf = fopen(path, "r")))
		{
		f = loadfont(pf);
		fclose(pf);
		}
	return (f);
	}

	
static FONT *try_locate_font(const CHAR *dir,
		const CHAR *name,
		const CHAR *suffix)
	{
	FONT *f = NULL;
	CHAR *path;
	CHAR *sep = (dir && strlen(dir)) ? "/" : "";

	assert(name && strlen(name));
	assert(suffix && strlen(suffix));

	if ((strlen(dir)+strlen(sep)+strlen(name)+1+strlen(suffix)) > PATH_MAX)
		return (NULL);

	path = mem_buy(NULL, PATH_MAX + 1, YES);
	sprintf(path, "%s%s%s.%s", dir, sep, name, suffix);
	if (!(f = try_load_font(path)))
		{
		sprintf(path, "%s%s%s", sep, dir, name);
		f = try_load_font(path);
		}

	mem_free(path);
	return (f);
	}


/* locate and open font-file, using $SLAP_FONTPATH if required...
 * If font-file is located, a loaded font is returned.
 * Otherwise NULL is returned.
 */
static FONT *locate_font(const CHAR *name, const CHAR *suffix)
	{
	FONT *f = NULL;
	CHAR *p;

	/* try as absolute path-prefix... */
	if ((f = try_locate_font("", name, suffix)))
		;
	/* try as relative path-prefix... */
	else if ((f = try_locate_font(".", name, suffix)))
		;
	/* try SLAP_FONTPATH directories as path-prefixes... */
	else if ((p = getenv("SLAP_FONTPATH")))
		{
		CHAR *d, *path;

		path = mem_buy(NULL, PATH_MAX + 1, YES);

		p = str_dup(p, YES);
		for (d = strtok(p, ":"); d && !f; d = strtok(NULL, ":"))
			{
			if (strlen(d) > PATH_MAX)
				continue;
			strcpy(path, d);
			/* zap any trailing slash... */
			if (path[strlen(path) - 1] == '/')
				path[strlen(path) - 1] = '\0';
			f = try_locate_font(path, name, suffix);
			}
		mem_free(p);
		}
	return (f);
	}


/* emit diagnostic messages in "lp.tell" format?
 * ie: pretend we are a LaserWriter?
 */
static BOOL lp_messages = NO;


/* This program must be able to operate in any of three modes:
 *
 *	(1) As a "fast filter" for the UNIX System V "lp" subsystem;
 *	(2) As a "printer-interface" agent for the UNIX BSD4 "lpr" subsystem;
 *	(3) As a standalone program.
 *
 * In cases (1) and (2) above, we assume that our stdout is already connected
 * to the required serial-port, but that the port is in an unknown state (in
 * the POSIX 1003.1 "termios" sense). We also assume that the port may have
 * open()-ed in write-only mode, which is OK for some printers, but not for
 * the SLP protocol, which requires a full-duplex channel. Thus we may have
 * to use fcntl() to make it both readable and writable.
 */
INT main(UINT ac, CHAR **av)
	{
	FILE *file;
	SHORT numcopies = 1;
	CHAR *face = "areal-10";
	CHAR *target = "-";		/* DO NOT CHANGE THIS INITIALISER! */
	CHAR *p;
	BOOL identify = NO;
	FONT *font;
	CHAR *modelname = "auto";
	CHAR *medianame = "std";
	BOOL portrait = NO;
	BOOL squeeze = NO;
	LONG maxbaudval = 0;
	SHORT baud_mapping_scheme = BAUDMAP_POSIX;
	SESSION *sd;

	getflags(&ac, &av, "b##,c#,f*,l,m*,o*,portrait,p*,s,version,x#",
		"[files]",
		&maxbaudval, &numcopies, &face, &lp_messages, &medianame,
		&target, &portrait, &modelname, &squeeze, &identify,
		&baud_mapping_scheme);
	if (identify)
		{
		identity();
		exit (EXIT_FAILURE);
		}
	if (!set_baud_mapping(baud_mapping_scheme))
		Error("invalid baud-rate mapping scheme (-x%h)",
			baud_mapping_scheme);

	#ifndef PATH_MAX
	set_path_max();
	#endif

	assert(target);
	sd = open_session(target, modelname, medianame, portrait, maxbaudval);

	p = (sd->drv.hres(sd->opd) > 7.0) ? "pro" : "plus";
	if (!(font = locate_font(face, p)))
		{
		Error("cannot locate font: %s.%s", face, p);
		}

	if (!ac || !*av)
		{
		CHAR buf[BUFSIZ];

		if (!(file = tmpfile()))
			Warning("cannot create temporary file");
		else
			{
			while (fgets(buf, BUFSIZ, stdin))
				fputs(buf, file);
			rewind(file);
			render(sd, file, numcopies, font, portrait, squeeze, NO);
			fclose(file);	/* which also removes it */
			}
		}

	for ( ; ac && *av; --ac, ++av)
		{
		if (!(file = fopen(*av, "r")))
			Warning("cannot read input file: %s", *av);
		else
			{
			render(sd, file, numcopies, font, portrait, squeeze, NO);
			fclose(file);	
			}
		}

	dropfont(font);
	close_session(sd);

	return (EXIT_SUCCESS);
	}

VOID Error(CHAR *fmt, ...)
	{
	va_list ap;

	va_start(ap, fmt);
	if (lp_messages)
		{
		fprintf(stderr, "%%%%[ error: ");
		vfprintf(stderr, fmt, ap);
		fprintf(stderr, " ]%%%%\n");
		}
	else
		verror(fmt, ap);
	va_end(ap);

	TRACE((5, "delay before kill"));
	sleep(4);
	kill(getpid(), SIGABRT);
	/* should not get to here! */
	exit (EXIT_FAILURE);
	}

VOID Warning(CHAR *fmt, ...)
	{
	va_list ap;

	va_start(ap, fmt);
	if (lp_messages)
		{
		fprintf(stderr, "%%%%[ error: ");
		vfprintf(stderr, fmt, ap);
		fprintf(stderr, " ]%%%%\n");
		}
	else
		vwarning(fmt, ap);
	va_end(ap);
	}

VOID Notice(CHAR *fmt, ...)
	{
	va_list ap;

	va_start(ap, fmt);
	if (lp_messages)
		{
		fprintf(stderr, "%%%%[ status: ");
		vfprintf(stderr, fmt, ap);
		fprintf(stderr, " ]%%%%\n");
		}
	else
		vremark(fmt, ap);
	va_end(ap);
	}
