/* slap: UNIX SmartLabel printing program, version 2
 * Module: mode- and session-state dispatcher
 *		(includes top-level signal-handling)
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
 **********************************************************************
 */
static const char * const module = "@(#)" __FILE__ " 2.1 30jan2001 MJS";


#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "mjsu.h"
#include "slap.h"

/* logical media sizes...
 */
static const MEDIA media[] =
	{
	{"std",		28, 89},
	{"euro",	36, 89},
	{"disk",	54, 70},
	{"ship",	54, 101},
	{"small",	28, 51},
	{"strip",	14, 87},
	{"slide",	38, 11},
	{"cass",	 9, 71},
	{"leitz",	36, 190},
	{"large",	51, 190}
	};
static const UINT nmedia = sizeof(media) / sizeof(media[0]);


static const INT sigs[] =
	{
	SIGHUP,
	SIGINT,
	SIGQUIT,
	SIGTERM,
	SIGABRT
	};
static const UINT nsigs = sizeof(sigs) / sizeof(sigs[0]);


static VOID handle_signal(INT sig, SESSION *sd)
	{
	struct sigaction action;

	/* we want to catch the signal (so we can shutdown the driver
	 * cleanly), but also want our invoker to get his usual indications
	 * about signalled child processes. The simplest way to do this
	 * is to catch the signal, handle it, stop catching the signal, and
	 * finally send the signal (again) to ourself! Neat, huh?
	 */
	TRACE((5, "killing session due to signal #%u", sig));

	/* tidy up:
	 */
	if (sd)
		close_session(sd);

	/* finally pretend that we did not catch the original signal,
	 * except for SIGABRT which is used internally by Error()...
	 */
	if ((sig != SIGABRT) && sigaction(sig, NULL, &action) >= 0)
		{
		sigset_t ss;

		/* NOTE: sig is blocked at this point (because we are inside
		 * the signal-handler for it!), so first change the handling
		 * for sig, then send it to ourself, and finally unblock it
		 * to allow delivery...
		 */
		sigdelset(&action.sa_mask, sig);
		sigcatch(sig, SIG_DFL, NULL, &action.sa_mask, NO);
		kill(getpid(), sig);

		/* enable re-delivery of signal by unblocking it...
		 */
		sigemptyset(&ss);
		sigaddset(&ss, sig);
		sigprocmask(SIG_UNBLOCK, &ss, NULL);
		}

	exit(EXIT_FAILURE);	/* just in case sigcatch or kill fails! */
	}


SESSION *open_session(const CHAR *target,
	const CHAR *modelname,
	const CHAR *medianame,
	BOOL portrait,
	LONG maxbaudval)
	{
	sigset_t mask;
	const MEDIA *m;
	SESSION *sd;
	UINT i;

	sd = mem_buy(NULL, sizeof(*sd), YES);
	memset(sd, 0, sizeof(*sd));

	for (i = 0, m = media; i < nmedia; ++i, ++m)
		{
		if (!strcmp(m->name, medianame))
			break;
		}
	if (i >= nmedia)
		Error("invalid media-type: %s", medianame);
	TRACE((4, "media size is %ux%u mm", m->height, m->width));


	/* set to catch some signals...so we can tidy up...
	 */
	sigemptyset(&mask);
	for (i = 0; i < nsigs; ++i)
		sigaddset(&mask, sigs[i]);
	for (i = 0; i < nsigs; ++i)
		{
		sigcatch(sigs[i],
			(VOID (*)(INT, VOID *)) handle_signal, sd, &mask, YES);
		}

	/* the driver and open-device-descriptor fields:
	 */
	if (!(sd->opd = load_driver(modelname, target, maxbaudval, &sd->drv)))
		Error("printer not responding");

	if (!sd->drv.setmedia(sd->opd, m, portrait, &sd->arena, NULL))
		{
		sd->drv.close(sd->opd);
		Error("invalid media-size for printer: %ux%u mm",
			m->height, m->width);
		}
	TRACE((4, "allocated %ux%u arena",
		mbm_height(sd->arena.bm),
		mbm_width(sd->arena.bm)));

	return (sd);
	}


VOID close_session(SESSION *sd)
	{
	if (sd)
		{
		if (sd->opd)
			sd->drv.close(sd->opd);
		if (sd->arena.bm)
			mbm_free(sd->arena.bm);
		sd = mem_free(sd);
		}
	}

