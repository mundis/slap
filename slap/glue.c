/* slap: UNIX SmartLabel printing program, version 2
 * Module: device-driver linkage-table and lookup routines
 *
 * Copyright (c) 2000, 2001 by Mike Spooner
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
 * Core API:    ANSI X3.159-1989 C Language
 * Other APIs:  MJS Portable C Library
 ***********************************************************************
 */
static const char * const module = "@(#)" __FILE__ " 1.2 30jan2001 MJS";


#include <string.h>
#include "slap.h"

/* structure mapping driver to an explicit driver-name:
 */
typedef struct
	{
	CHAR *name;		/* eg: "plus" or "slp-pro" or "X:slp-220" */
	PTYPE tentative_ptype;	/* see also the ptype member of OPD */
	BOOL probe;		/* this entry can be used during auto-probe */
	DRV *drv;		/* device-driver */
	DRV *chain;		/* optional chained device-driver */
	} DRIVERMAP;


extern DRV slplus_drv, slpro_drv, mbm_drv;

static const DRIVERMAP drivers[] =
	{
	{"slp",		PT_SLP,		YES,	&slplus_drv,	NULL},
	{"mbm:slp",	PT_SLP,		NO,	&mbm_drv,	&slplus_drv},
	{"slp/plus",	PT_SLP_PLUS,	NO,	&slplus_drv,	NULL},
	{"mbm:slp/plus",PT_SLP_PLUS,	NO,	&mbm_drv,	&slplus_drv},
	{"plus",	PT_SLP_PLUS,	NO,	&slplus_drv,	NULL},
	{"slp/ez30",	PT_SLP_EZ30,	NO,	&slplus_drv,	NULL},
	{"mbm:slp/ez30",PT_SLP_EZ30,	NO,	&mbm_drv,	&slplus_drv},
	{"slp/pro",	PT_SLP_PRO,	YES,	&slpro_drv,	NULL},
	{"mbm:slp/pro",	PT_SLP_PRO,	NO,	&mbm_drv,	&slpro_drv},
	{"pro",		PT_SLP_PRO,	YES,	&slpro_drv,	NULL},
#if 0
	{"X:slp/pro",	PT_SLP_PRO,	NO,	&xwin_drv,	&slpro_drv},
#endif
	{"slp/100",	PT_SLP_100,	NO,	&slpro_drv,	NULL},
	{"mbm:slp/100",	PT_SLP_100,	NO,	&mbm_drv,	&slpro_drv},
	{"slp/120",	PT_SLP_120,	NO,	&slpro_drv,	NULL},
	{"mbm:slp/120",	PT_SLP_120,	NO,	&mbm_drv,	&slpro_drv},
	{"slp/200",	PT_SLP_200,	NO,	&slpro_drv,	NULL},
	{"mbm:slp/200",	PT_SLP_200,	NO,	&mbm_drv,	&slpro_drv},
	{"slp/220",	PT_SLP_220,	NO,	&slpro_drv,	NULL},
	{"mbm:slp/220",	PT_SLP_220,	NO,	&mbm_drv,	&slpro_drv},
	{"slp/240",	PT_SLP_240,	NO,	&slpro_drv,	NULL},
	{"mbm:slp/240",	PT_SLP_240,	NO,	&mbm_drv,	&slpro_drv}
	};
static const UINT ndrivers = sizeof(drivers) / sizeof(drivers[0]);


OPD *load_driver(const CHAR *name, const CHAR *target, LONG maxbaudval,
	DRV *drv)
	{
	OPD *opd;
	UINT i;

	for (i = 0; i < ndrivers; ++i)
		{
		/* a NULL name or a name of "auto" means to auto-probe,
		 * giving us a MAJOR composite conditional-test... :)
		 */
		if ((((!name || !strcmp(name, "auto")) && drivers[i].probe) ||
			!strcmp(name, drivers[i].name)) &&
				(opd = drivers[i].drv->open(target, maxbaudval,
					drivers[i].tentative_ptype,
					drivers[i].chain)))
			{
			*drv = *drivers[i].drv;
			return (opd);
			}
		}
	return (NULL);
	}


#if DEBUG_LEVEL > 0

VOID dump_opd(const CHAR *prefix, OPD *opd)
	{

	TRACE((4, "%s: OPD = { %i, %s, %p }",
		prefix,
		opd->ptype,
		opd->target ? opd->target : "null",
		opd->private));
	}
#endif
