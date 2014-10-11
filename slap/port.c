/* slap: UNIX SmartLabel printing program, version 2
 * Module: general-purpose port-control functions
 *
 * Copyright (c) 1997-1998, 2001 by Mike Spooner
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
static const char * const module = "@(#)" __FILE__ " 1.2 30jan2001 MJS";


#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE
#endif

#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <time.h>
#include "mjsu.h"
#include "port.h"


/* test if file-descriptor is open for reading
 */
BOOL is_readable(INT fd)
	{
	INT om;

	return (((om = fcntl(fd, F_GETFL)) != -1) && (om & (O_RDONLY|O_RDWR)));
	}

/* test if file-descriptor is open for writing
 */
BOOL is_writable(INT fd)
	{
	INT om;

	if ((om = fcntl(fd, F_GETFL)) != -1)
		{
		if (om & (O_WRONLY|O_RDWR))
			return (YES);
		}
	return (NO);
	}

	
/* get an exclusive lock on a file (if possible), with timeout in seconds.
 * If successful, returns YES, else returns NO and sets errno.
 * (errno == EAGAIN) indicates that the timeout expired.
 */
BOOL fxlock(INT fd, INT timeout)
	{
	time_t t1;
	struct flock lck;

	lck.l_type = F_WRLCK;
	lck.l_start = lck.l_len = 0;	/* entire file */
	lck.l_whence = SEEK_SET;

	if (timeout < 0)
		timeout = 0;

	t1 = time(NULL);
	do
		{
		errno = 0;
		if (fcntl(fd, F_SETLK, &lck) != -1)
			{
			/* success! */
			return (YES);
			}
		else if (errno != EAGAIN)
			{
			/* cannot lock this kind of file */
			return (NO);
			}
		sleep(1);
		} while ((time(NULL) - t1) < timeout);

	/* timed-out if get to here... */
	return (NO);
	}


/* test if an open file refers to the process' controlling terminal.
 */
BOOL isctty(INT fd)
        {
        if (tcgetpgrp(fd) != -1)
                return (YES);
        else
                return (NO);
        }
 

/* open a serial-port device file, without possibly nasty side-effects.
 * If successful, a newly-open file-descriptor is returned. Otherwise, -1
 * is returned and errno is set to reflect the error.
 * The semantics of open() are used, except that:
 *
 *	(1) the file is opened without waiting for DCD to be asserted;
 *
 *	(2) the file is opened with signal-on-serial-break (termios BRKINT) 
 *	    disabled, and the open is done atomically with respect to SIGINT.
 *
 *	(3) flags which do not make sense for serial-port devices are ignored
 *	    (O_CREAT, O_EXCL, O_TRUNC, O_APPEND).
 *
 *	(4) Upon successful open, the serial-port's initial termios modes
 *	    (before suppression of BRKINT) are recorded at *saved_termios,
 *	    so that the caller can restore the extant port settings before
 *	    exit()-ing or close()-ing the file. NOTE: closetty() does such
 *	    a restore atomically with respect to SIGINT.
 *
 *	(5) The error ENOTTY is delivered if the specified name does not
 *	    refer to a serial-port device.
 *
 *	(6) If the file name is NULL, empty or "-", a new file-descriptor
 *	    referring to the same file as standard-output is generated,
 *	    rather than actually open()-ing the file.
 */
INT opentty(const CHAR *name, INT oflag, struct termios *save)
	{
	INT fd;
	struct termios tm;
	sigset_t mask, oldmask;

	assert(name && *name);

	oflag &= ~(O_CREAT|O_EXCL|O_TRUNC|O_APPEND);

	/* must block SIGINT to avoid possible interrruption between
	 * open() and supression of termios BRKINT mode...
	 * NOTE: must ensure that we can restore the current signal mask
	 * when finished.
	 */
	sigemptyset(&mask);
	sigemptyset(&oldmask);
	sigaddset(&mask, SIGINT);
	sigprocmask(SIG_BLOCK, &mask, &oldmask);

	if (!strcmp(name, "-"))
		{
		fd = dup(STDOUT_FILENO);
		switch (oflag & O_ACCMODE)
			{
		case O_RDONLY:
			if (!is_readable(fd))
				{
				sigprocmask(SIG_SETMASK, &oldmask, NULL);
				close(fd);
				return (-1);
				}
			break;
		case O_WRONLY:
			if (!is_writable(fd))
				{
				sigprocmask(SIG_SETMASK, &oldmask, NULL);
				close(fd);
				return (-1);
				}
			break;
		default:
			if (!is_readable(fd) || !is_writable(fd))
				{
				sigprocmask(SIG_SETMASK, &oldmask, NULL);
				close(fd);
				return (-1);
				}
			break;
			}
		}
	else
		{
		/* initially open with explicit O_NONBLOCK to avoid waiting for
		 * carrier-detect during open():
		 */
		if ((fd = open(name, oflag|O_NONBLOCK, 0)) < 0)
			{
			sigprocmask(SIG_SETMASK, &oldmask, NULL);
			return (-1);
			}
		}

	/* now set desired file-status flags:
	 */
	oflag &= ~O_ACCMODE;
	if (fcntl(fd, F_SETFL, oflag) < 0)
		{
		sigprocmask(SIG_SETMASK, &oldmask, NULL);
		close(fd);
		return (-1);
		}

	/* record original termios modes, for restore-on-close; and turn off
	 * BRKINT mode on the port; ....notice that this will catch
	 * the ENOTTY error as a handy side effect.
	 */
	if ((tcgetattr(fd, save) < 0) || (tcgetattr(fd, &tm) < 0))
		{
		sigprocmask(SIG_SETMASK, &oldmask, NULL);
		close(fd);
		return (-1);
		}
	tm.c_iflag &= ~BRKINT;
	if (tcsetattr(fd, TCSAFLUSH, &tm) < 0)
		{
		sigprocmask(SIG_SETMASK, &oldmask, NULL);
		close(fd);
		return (-1);
		}
	tcflush(fd, TCIOFLUSH);	/* paranoia! */

	/* now is safe to unblock SIGINT:
	 */
	sigprocmask(SIG_SETMASK, &oldmask, NULL);

	/* all done! */
	return (fd);
	}

BOOL closetty(INT fd, const struct termios *restore)
	{
	sigset_t mask, oldmask;

	/* must first block SIGINT to avoid possible interrruption between
	 * restoral of termios modes and the final close()...
	 * NOTE: must ensure that we can restore the current signal mask
	 * when finished.
	 */
	sigemptyset(&mask);
	sigemptyset(&oldmask);
	sigaddset(&mask, SIGINT);
	sigprocmask(SIG_BLOCK, &mask, &oldmask);

	/* restore termios modes:
	 */
	tcflow(fd, TCOON);	/* better would be to turn off flow-control! */
	if (tcsetattr(fd, TCSADRAIN, (struct termios *) restore) < 0)
		{
		sigprocmask(SIG_SETMASK, &oldmask, NULL);
		return (NO);
		}

	/* close the port:
	 * NOTE: some UNIX serial UARTs or device-drivers will hard-hang in
	 * close() if the ports transmitter is disabled by flow-control,
	 * EVEN IF THERE ARE NO OUTPUT BYTES QUEUED FOR TRANSMISSION!!!
	 * Thus we must turn off flow-control before calling close()...
	 */
	tcflow(fd, TCOON);
	if (close(fd) < 0)
		{
		sigprocmask(SIG_SETMASK, &oldmask, NULL);
		return (NO);
		}

	/* now is safe to unblock SIGINT:
         */
        sigprocmask(SIG_SETMASK, &oldmask, NULL);

	return (YES);
	}


/* tables mapping baud-rate numbers into symbolic constants usable
 * with tcsetattr(), or names usable in diagnoctic messages.
 *
 * There are several mappings, to support various port-specific
 * (driver-specific) mappings of termios Bxxxxx symbolic baud-rates
 * into actual baud-rate semantics.
 * Note that each such table must be terminated by an entry with
 * a negative baud-rate-value.
 * The tables here are general purpose, but are *ordered* to provide
 * the fastest linear lookup for slap (ie: with the common baud-rates
 * used by slap near the top).
 */
typedef struct
	{
	LONG n;
	speed_t b;
	const char *s;
	} BAUDINFO;

/* symbolic baud-rate mapping for POSIX 1003.1 with optional high-speed
 * symbols - no actual mapping of low-to-high baud-rates.
 */
static const BAUDINFO generic_map[] =
	{
	{9600,		B9600,		"9600"},
	{19200,		B19200,		"19200"},
	{38400,		B38400,		"38400"},
#if defined(B57600)
	{57600,		B57600,		"57600"},
#endif
	{0,		B0,		"0"},
	{50,		B50,		"50"},
	{75,		B75,		"75"},
	{110,		B110,		"110"},
	{134,		B134,		"134.5"},
	{150,		B150,		"150"},
	{200,		B200,		"200"},
	{300,		B300,		"300"},
	{600,		B600,		"600"},
	{1200,		B1200,		"1200"},
	{2400,		B2400,		"2400"},
	{4800,		B4800,		"4800"},
#if defined(B76800)
	{76800,		B76800,		"76800"},
#endif
#if defined(B115200)
	{115200,	B115200,	"115200"},
#endif
#if defined(B153600)
	{153600,	B153600,	"153600"},
#endif
#if defined(B230400)
	{230400,	B230400,	"230400"},
#endif
#if defined(B307200)
	{307200,	B307200,	"307200"},
#endif
#if defined(B460800)
	{460800,	B460800,	"460800"},
#endif
	{-1,		(speed_t) -1,	NULL}		/* terminator */
	};

/* symbolic baud-rate mapping for AIX 4.x on the RS/6000 when using
 * ports on the 7318 8-port concentrator or the 128-port adaptor
 * which have been initialised with the "stty-cxma fastbaud" command.
 */
static const BAUDINFO aix4_fastbaud_map[] =
	{
	{9600,		B9600,		"9600"},
	{19200,		B19200,		"19200"},
	{38400,		B38400,		"38400"},
	{57600,		B50,		"57600"},
	{0,		B0,		"0"},
	{75,		B75,		"75"},
	{110,		B110,		"110"},
	{134,		B134,		"134.5"},
	{150,		B150,		"150"},
	{200,		B200,		"200"},
	{300,		B300,		"300"},
	{600,		B600,		"600"},
	{1200,		B1200,		"1200"},
	{2400,		B2400,		"2400"},
	{4800,		B4800,		"4800"},
	{-1,		(speed_t) -1,	NULL}		/* terminator */
	};

/* symbolic baud-rate mapping for SunOS 4.x and Solaris 2.1-2.4 when
 * using Magma 4SP, 8SP and 16SP (including the SYNC and DMA variant)
 * SBus serial-port cards.
 */
static const BAUDINFO magma_fastbaud_map[] =
	{
	{9600,		B9600,		"9600"},
	{19200,		B19200,		"19200"},
	{38400,		B38400,		"38400"},
	{57600,		B75,		"57600"},
	{0,		B0,		"0"},
	{110,		B110,		"110"},
	{150,		B150,		"150"},
	{300,		B300,		"300"},
	{600,		B600,		"600"},
	{4800,		B4800,		"4800"},
	{56000,		B50,		"56000"},
	{64000,		B134,		"64000"},
	{76800,		B200,		"76800"},
	{96000,		B1200,		"96000"},
	{115200,	B1800,		"115200"},
	{128000,	B2400,		"128000"},
	{-1,		(speed_t) -1,	NULL}		/* terminator */
	};


/* the current baud-mapping, and the default setting
 */
static const BAUDINFO *baudmap = generic_map;


/* function to set the baud mapping in effect
 */
BOOL set_baud_mapping(const BAUD_MAPPING scheme)
	{
	switch (scheme)
		{
	case BAUDMAP_MAGMA_FAST:
		baudmap = magma_fastbaud_map;
		break;
	case BAUDMAP_AIX4_FAST:
		baudmap = aix4_fastbaud_map;
		break;
	case BAUDMAP_NONE:
	case BAUDMAP_POSIX:
		baudmap = generic_map;
		break;
	default:
		return (NO);
		break;
		}
	return (YES);
	}


const CHAR *baud2name(speed_t b)
	{
	UINT i;

	for (i = 0; baudmap[i].n >= 0; ++i)
		{
		if (baudmap[i].b == b)
			return (baudmap[i].s);
		}
	return ("unknown baud rate");
	}

speed_t val2baud(LONG val)
	{
	UINT i;

	for (i = 0; baudmap[i].n >= 0; ++i)
		{
		if (baudmap[i].n == val)
			return(baudmap[i].b);
		}
	return (B0);
	}

LONG baud2val(speed_t b)
	{
	UINT i;

	for (i = 0; baudmap[i].n >= 0; ++i)
		{
		if (baudmap[i].b == b)
			return (baudmap[i].n);
		}
	return (0);
	}

/* compare two speed_t symbols, in a manner similar to strcmp()
 */
INT baudcmp(speed_t x, speed_t y)
	{
	UINT i, j;

	for (i = 0; baudmap[i].n >= 0; ++i)
		if (x == baudmap[i].b)
			break;
	
	for (j = 0; baudmap[j].n >= 0; ++j)
		if (y == baudmap[j].b)
			break;

	return ((i == j) ? 0 :
		(baudmap[i].n < baudmap[j].n) ? -1 :
		1);
	}

/* enable CTS transmit flow-control on an open file.
 * Returns YES if successful, else NO.
 */
BOOL use_cts(INT fd)
	{
	#if !defined(CRTSCTS)
	return (NO);
	#else
	struct termios tm;

	if (tcgetattr(fd, &tm) < 0)	/* eg: ENOTTY */
		return (NO);
	tm.c_cflag |= CRTSCTS;
	if (tcsetattr(fd, TCSANOW, &tm) < 0)	/* eg: not valid for this file */
		return (NO);
	return (YES);
	#endif
	}

/* disable CTS transmit flow-control on an open file.
 * Returns YES if successful, else NO.
 */
BOOL avoid_cts(INT fd)
	{
	#if !defined(CRTSCTS)
	return (YES);
	#else
	struct termios tm;

	if (tcgetattr(fd, &tm) < 0)	/* eg: ENOTTY */
		return (NO);
	tm.c_cflag &= ~CRTSCTS;
	if (tcsetattr(fd, TCSANOW, &tm) < 0)	/* eg: not valid for this file */
		return (NO);

	/* should have succeeded if get to here, but let's just make sure...
	 */
	tcgetattr(fd, &tm);
	if (tm.c_cflag & CRTSCTS) /* whoops! CTS flow-control still active */
		return (NO);

	return (YES);
	#endif
	}


/* can we enable CTS transmit-flow-control on the specified open file?
 * Return YES or NO.
 */
BOOL try_cts(INT fd)
	{
	#if !defined(CRTSCTS)
	return (NO);
	#else
	struct termios tm;

	/* NOTE: be very careful not to disturb any termios settings
	 * or queued data on the file-descriptor...
	 */

	if (tcgetattr(fd, &tm) < 0)	/* eg: ENOTTY */
		return (NO);

	else if ((tm.c_cflag & CRTSCTS) && (tcsetattr(fd, TCSANOW, &tm) == 0))
		return (YES);

	else if (use_cts(fd))
		{
		avoid_cts(fd);
		return (NO);
		}

	avoid_cts(fd);
	return (YES);
	#endif
	}


