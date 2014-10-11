/* slap: UNIX SmartLabel printing program, version 2
 * Module: definitions of common data-types, macros, manifest constants
 *
 * Copyright (c) 1997-1998, 2000, 2001 by Mike Spooner
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
 *		MJS BitFont Library
 *		IEEE Std 1003.1-1990 (aka POSIX.1)
 ****************************************************************************
 */
#ifndef __SLAP_H__
#define __SLAP_H__ 1

static const char * const slap_h = "@(#)" __FILE__ " 2.8 30jan2001 MJS";


#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <assert.h>
#include "mjsfont.h"


#define DEBUG_LEVEL 0

#if DEBUG_LEVEL
VOID trace(UINT level, CHAR *fmt, ...);
#define TRACE(args)	trace args 
#else
#define TRACE(args)	
#endif

/* abstract printer-model codes:
 */
typedef enum
	{
	PT_NONE=0,
	PT_SLP_240,
	PT_SLP,
	PT_SLP_PLUS,
	PT_SLP_EZ30,
	PT_SLP_PRO,
	PT_SLP_120,
	PT_SLP_220,
	PT_SLP_100,
	PT_SLP_100N,
	PT_SLP_200,
	PT_SLP_200N
	} PTYPE;


/* media-type specifications:
 */
struct media_
	{
	CHAR *name;
	BYTE width;		/* in mm */
	BYTE height;		/* in mm */
	};
typedef struct media_ MEDIA;


/* structures representing physical layout of dots on the labels:
 */
struct arena_
	{
	MBITMAP *bm;
	BOOL dirty;
	};
typedef struct arena_ ARENA;

/* an open-printer-descriptor (represents a comms session with the printer)
 */
struct opd_
	{
	PTYPE ptype;		/* the actual (detected) printer model */
	CHAR *target;		/* the device-name used to access the printer,
				 * or NULL if running a disconnected session */
	VOID *private;		/* session-data private to the device-driver */
	};
typedef struct opd_ OPD;

/* structure describing the geometry of a media type, in pixels
 */
struct geometry_
	{
	USHORT top, bottom, left, right;	/* unprintable margins */
	USHORT width, height;			/* of printable area */
	};
typedef struct geometry_ GEOMETRY;

/* static device-driver description:
 */
struct device_driver
	{
	/* Until the "open" member has been called (and returned success), or
	 * after the "close" member has been called, only the "open" member
	 * is valid. The open() member is the only way to create a valid OPD.
	 * close() implicitly destroys the OPD.
	 */
	OPD *(*open)(const CHAR *target, LONG maxbaudval,
			PTYPE tentative, struct device_driver *chain);
	BOOL (*setmedia)(OPD *, const MEDIA *, BOOL, ARENA *, GEOMETRY *);
	VOID (*print)(OPD *, const ARENA *);
	VOID (*close)(OPD *);	/* could be called asynchronously */
	DOUBLE (*hres)(OPD *);	/* in dots-per-millimetre */
	DOUBLE (*vres)(OPD *);	/* in dots-per-millimetre */
	};
typedef struct device_driver DRV;


/* a "slap" session descriptor:
 */
struct session_
	{
	DRV drv;
	ARENA arena;
	OPD *opd;
	};
typedef struct session_ SESSION;


/* Abstract print-density codes:
 */
typedef enum
	{
	DENS_LIGHT, DENS_DEMILIGHT, DENS_NORM, DENS_DEMIBOLD, DENS_BOLD
	} DENSITY;


/* Global function declarations:
 */
SESSION *open_session(const CHAR *target, const CHAR *model, const CHAR *media,
		BOOL portrait, LONG maxbaudval);
VOID close_session(SESSION *sd);
VOID render(SESSION *sd, FILE *pf, UINT nc, FONT *font,
		BOOL portrait, BOOL squeeze, BOOL boxed);
INT main(UINT ac, CHAR **av);
VOID identity(VOID);
VOID Error(CHAR *, ...);
VOID Warning(CHAR *, ...);
VOID Notice(CHAR *, ...);
INT sigcatch(INT sig, VOID (*handler)(INT, VOID *), VOID *arg,
		const sigset_t *mask, BOOL force);


/* device-driver linkage routines:
 */
OPD *load_driver(const CHAR *name,
		const CHAR *target, LONG maxbaudval, DRV *drv);
VOID dump_opd(const CHAR *prefix, OPD *opd);


/* global data:
 */
extern BOOL debugbox;
#ifndef PATH_MAX
extern LONG PATH_MAX;
#endif

#endif
