/* slap: UNIX SmartLabel printing program, version 2
 * Module: portable enhanced signal-handling facilities
 *
 * Copyright (c) 1997-1998 by Mike Spooner
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
 **********************************************************************
 */
static const char * const module = "@(#)" __FILE__ " 1.0 05jun1998 MJS";


#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include "mjsu.h"
#include "slap.h"


struct action
	{
	struct action *next;
	INT sig;
	VOID (*handler)(INT sig, VOID *arg);
	VOID *arg;
	};
typedef struct action ACTION;

static ACTION *actions = NULL;


static VOID chain_handler(INT sig)
	{
	const ACTION *p;

	if (sigaction(sig, NULL, NULL) < 0)
		return;
	for (p = actions; p; p = p->next)
		{
		if ((p->sig == sig) && p->handler)
			{
			p->handler(sig, p->arg);
			return;
			}
		}
	}


INT sigcatch(INT sig, VOID (*handler)(INT, VOID *), VOID *arg,
	const sigset_t *mask, BOOL force)
	{
	sigset_t tmpmask, oldmask;
	struct sigaction x;
	ACTION *p;

	/* check arguments:
	 */
	if (sigaction(sig, NULL, &x) < 0)
		return (-1);
	if (!force && (x.sa_handler == SIG_IGN))
		{
		errno = EACCES;
		return (-1);
		}

	/* block the signal whilst changing it's handling!
	 */
	sigemptyset(&tmpmask);
	sigemptyset(&oldmask);
	sigaddset(&tmpmask, sig);
	sigprocmask(SIG_BLOCK, &tmpmask, &oldmask);
	
	/* change handling of sig:
	 */
	sigemptyset(&x.sa_mask);
	if (mask)
		x.sa_mask = *mask;
	x.sa_flags = 0;

	if ((handler == SIG_DFL) || (handler == SIG_IGN))  /* not catching! */
		x.sa_handler = handler;
	else	/* catching! */
		{
		x.sa_handler = chain_handler;
		for (p = actions; p; p = p->next)
			{
			if (p->sig == sig)
				break;
			}
		if (!p)
			{
			p = mem_buy(NULL, sizeof(*p), YES);
			p->next = actions;
			actions = p;
			}
		p->sig = sig;
		p->handler = (VOID (*)(INT, VOID *))handler;
		p->arg = arg;
		}
	sigaction(sig, &x, NULL);

	/* and release and pending occurrences of sig:
	 */
	sigprocmask(SIG_SETMASK, &oldmask, NULL);

	return (0);
	}
	
