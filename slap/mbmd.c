/* slap: UNIX SmartLabel printing program, version 2
 * Module: command/communication-driver for bitmap dump-file
 *
 * Copyright (c) 1997, 2000, 2001 by Mike Spooner
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
 *		IEEE Std 1003.1-1990 (aka POSIX.1)
 ***********************************************************************
 */
static const char * const module = "@(#)" __FILE__ " 2.2 30jan2001 MJS";


#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "mjsu.h"
#include "slap.h"


/* structure representing private driver-state data:
 */
typedef struct
	{
	FILE *pf;
	const CHAR *fname;
	GEOMETRY g;	/* in logical orientation */
	MBITMAP *bm;	/* in logical orientation, including margins */
	DRV *drv;	/* the underlying printer-specific device-driver */
	OPD *opd;	/* the open device descriptor for the above */
	} STATE;


static VOID printlabel(OPD *opd, const ARENA *a)
	{
	STATE *st = opd->private;
	#if 0
	USHORT r, c;

	/* put faint lines around the edge of the printable region:
	 */
	for (r = st->g.top;
		r < (st->g.top + mbm_height(a->bm));
		r += 3)
		{
		c = st->g.left;
		mbm_setb(bm, r, c);
		mbm_setb(bm, r, c + mbm_width(a->bm));
		}
	for (c = st->g.left;
		c < (st->g.left + mbm_width(a->bm));
		c += 3)
		{
		r = st->g.top;
		mbm_setb(bm, r, c);
		mbm_setb(bm, r + mbm_height(a->bm), c);
		}
	#endif

	/* and append bitmap into output file:
	 */
	if (!mbm_save(a->bm, st->pf))
		Error("write error on %s", st->fname);
	return;
	}


/* open the "printer"
 * NOTE: a target of "-" means use standard-output.
 */
static OPD *opendev(const CHAR *target, LONG maxbaudval,
	PTYPE tentative_ptype, DRV *chain)
	{
	OPD *opd, *opd2;
	STATE *st = NULL;

	if (!(opd2 = chain->open(NULL, maxbaudval, tentative_ptype, NULL)))
		return (NULL);

	opd = mem_buy(NULL, sizeof(*opd), YES);
	opd->ptype = tentative_ptype;
	opd->private = NULL;

	if (target)
		{
		st = mem_buy(NULL, sizeof(*st), YES);
		memset(st, 0, sizeof(*st));
		st->fname = NULL;
		st->pf = NULL;
		st->drv = chain;
		st->opd = opd2;
	
		if (strcmp(target, "-"))
			{
			if (!(st->pf = fopen(target, "w+b")))
				Error("cannot create output file: %s", target);
			opd->target = str_dup(target, YES);
			}
		else
			{
			st->pf = stdout;
			opd->target = str_dup("standard-output", YES);
			}
		st->fname = opd->target;	/* NOTE: this is an alias */
		opd->private = st;
		}
	else
		opd->target = NULL;

	return (opd);
	}

static BOOL setmedia(OPD *opd, const MEDIA *m, BOOL portrait,
	ARENA *a, GEOMETRY *g)
	{
	STATE *st = opd->private;

	assert(opd->target);

	if (st->drv->setmedia(st->opd, m, portrait, NULL, &st->g))
		{
		if (g)
			*g = st->g;

		if ((portrait && (st->g.height < st->g.width)) ||
			(!portrait && (st->g.height > st->g.width)))
			{
			GEOMETRY alt = st->g;

			/* rotate geometry */
			st->g.height = alt.width;
			st->g.width = alt.height;
			st->g.top = alt.right;
			st->g.bottom = alt.left;
			st->g.left = alt.top;
			st->g.right = alt.bottom;
			}

		if (st->bm)
			mbm_free(st->bm);
		st->bm = mbm_buy(st->g.height + st->g.top + st->g.bottom,
				st->g.width + st->g.left + st->g.right, YES);
		if (a)
			{
			if (a->bm)
				mbm_free(a->bm);
			a->bm = mbm_buy(st->g.height, st->g.width, YES);
			a->dirty = NO;
			}
		return (YES);
		}
	return (NO);
	}

/* close down the "printer"
 */
static VOID closedev(OPD *opd)
	{
	STATE *st = opd->private;

	#if DEBUG_LEVEL > 1
	dump_opd("chained opd", st->opd);
	#endif
	if (opd->target)
		{
		st->drv->close(st->opd);
		if (st->pf != stdout)
			fclose(st->pf);
		st->pf = NULL;
		if (st->bm)
			mbm_free(st->bm);
		mem_free(st);
		mem_free(opd->target);
		}
	mem_free(opd);
	}

static DOUBLE hres(OPD *opd)
	{
	STATE *st = opd->private;

	return (st->drv->hres(st->opd));
	}

static DOUBLE vres(OPD *opd)
	{
	STATE *st = opd->private;

	return (st->drv->vres(st->opd));
	}


DRV mbm_drv =
	{
	opendev,
	setmedia,
	printlabel,
	closedev,
	hres,
	vres
	};
