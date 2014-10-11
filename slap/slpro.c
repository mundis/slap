/* slap: UNIX SmartLabel printing program, version 2
 * Module: display-driver for SLP-2000 printer and descendants
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
 *		IEEE Std 1003.1-1990 (aka POSIX.1)
 *
 * NOTE: specific common extensions to POSIX.1 "termios" are enabled if
 * supported by the host system: CTS flow-control, and 57600 baud.
 ***********************************************************************
 *
 * This driver supports the following Seiko SmartLabel printers:
 *
 *		Model No.	Product Name
 *		--------------------------------------------
 *		2000		Smart Label Printer Pro
 *		100		Smart Label Printer 100
 *		100N		Smart Label Printer 100N
 *		120		Smart Label Printer 120
 *		200		Smart Label Printer 200
 *		200N		Smart Label Printer 200N
 *		220		Smart Label Printer 220
 *		240		Smart Label Printer 240
 *
 *
 * NOTE:
 *
 * The USB-capable models must be connected via their serial-port, not USB.
 * UNIX APIs for USB are very thin on the ground right now.
 *
 * This code assumes that the character-special device-file for the
 * serial-port is operating in "termios" mode as per POSIX 1003.1.
 * Many UNIX systems provide optional alternative port configurations,
 * selectable dynamically.
 *
 * You may also need to get your system administrator to configure the
 * serial-port device-file as either a "Call-Up-Access" or "modemless"
 * port. Some UNIX systems provide alternative serial-port device-files
 * which are specifically pre-configured in this manner, with names like
 * "/dev/cua0" or "/dev/cua/a".
 *
 * Note: these SmartLabel printers come with an RJ11/DB25 null-modem
 * cable, thus as far as we are concerned, the printer is a DCE device.
 *
 ***********************************************************************
 */
static const char * const module = "@(#)" __FILE__ " 2.11 09feb2001 MJS";


#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include "mjsu.h"
#include "slap.h"
#include "port.h"


/* These printers must be connected via an RS-232 serial port.
 * They have a RJ11 port which has a null-modem RJ11/DB25 cable fitted.
 * This combination acts as a DCE (a modem!) supporting the
 * following RS-232 signals:
 *
 *	TX	transmitted data	(input)
 *	RX	received data		(output)
 *	DSR	data-set ready		(output)	SLP-2000 only
 *	CTS	clear-to-send		(output)	not on SLP-2000
 *	GND	reference ground voltage
 *
 * Flow-control is unidirectional (the printer assumes that it always OK
 * to transmit to the computer). Flow-control of the computer by the printer
 * is done using both XON/XOFF and DTR (SLP-2000) or CTS (SLP-1X0/2X0)
 * signalling.
 *
 * Very few UNIX systems directly support DSR-based transmit flow-control,
 * so in the case of the SLP-2000, we are restricted to XON/XOFF flow-control.
 * The intent is that this program should NEVER require anyone to build a
 * custom cross-over RS-232 cable: the printer must be usable "out of the box".
 *
 * For the SLP-1X0 and SLP-2X0, we utilise CTS flow-control if supported
 * by the host system, otherwise fall back to XON/XOFF. NOTE: when resorting
 * to XON/XOFF, we also limit the baud-rate to 38400 during the data-transfer
 * phase.
 *
 * NOTE: even at 38400 baud, XON/XOFF flow-control is safe even for
 * computers with poor serial UARTs (eg: old PC with Intel 16450 UART),
 * because the Pro-family printers internally pace their transmissions
 * to a maximum of 1 byte every 10 milliseconds, to avoid UART
 * buffer-overruns in the host computer.
 *
 *****************************************************************************
 *
 * The Seiko documentation claims that all printers in the "pro" family have
 * the same horizontal and vertical dot-pitch - but they don't...
 *
 * The SLP-2000 has a slightly different vertical pitch - it is not quite
 * "square". At least the ones I have had were like this...
 *
 *****************************************************************************
 *
 * These printers provide a simple "status-bits" protocol (much simpler than
 * the protocol used by the SLP-1x00 series).
 *
 * The various status bits returned by the printer come in two distinct
 * flavours: exceptions and progress-reports. Exceptions are asynchronous
 * protocol events, progress-reports are synchronous (ie: you know when to
 * expect them, they occur in a predictable sequence at a predictable point
 * in the conversation).
 *
 * The only progress-report available on all the models is STAT_IDLE,
 * which indicates that the printer has completed ALL commands so far sent.
 * A whole sequence of commands can be sent (subject to flow-control)
 * and STAT_IDLE will only be signalled when all of them are complete.
 *
 * The SLP-1X0 and SLP-2X0 also provide a "checkpoint" progress-report via
 * an "echo" command. That facility is not used by this driver (no need).
 *
 * In general, the printer only sends an unsolicited status-byte when it's
 * status actually changes.
 *
 * However, the SLP-2000 has a firmware bug whereby the printer checks for
 * the presence of labels ONLY upon receipt of one of the "feed" commands
 * (or when switched on), rather than all the time. Even an explicit
 * "get status" command does not in itself cause the printer to check that
 * labels are loaded.
 *
 * I am not sure if the SLP-1X0/2X0 printers have the same bug, but will
 * assume so until proven otherwise.
 *
 * Thus it is only possible to test whether labels are loaded
 * in a painfully restricted set of situations.
 *
 *****************************************************************************
 *
 * Serial-Port (termios) Modes:
 *
 * Because the DCD signal is not provided by the printer, we will
 * operate the port in CLOCAL mode.
 *
 * Flow-control modes are dependant on the capabilities of the printer model
 * and host system, as described above.
 *
 * These printers assume transparent byte-wide i/o, so we must
 * disable many of the common default serial-driver translations (echo, cr/nl,
 * parity-marking, lower/upper case, printable representation, strip-to-7-bit,
 * and all that stuff).
 * We also need non-canonical-mode so that we can perform timed reads.
 *
 * These printers serial-ports are fully asyncronous, in the RS-232 sense:
 * they neither provide nor use external clocking.
 *
 * These printers' baud-rate defaults to either 9600 or 57600, but can be
 * changed by an explicit command. Thus we initially "hunt" for the current
 * printer baud rate and then switch it to our preferred baud rate.
 *
 * These printers' serial ports operates with a fixed framing (1 start bit,
 * 8 data bits, 1 stop bits; no parity).
 *
 *****************************************************************************
 *
 * Platform Notes: PC/Linux
 *
 * Under PC-Linux, tcdrain() can return before all queued output data has
 * been physically transmitted. The result is that issuing tcdrain() and
 * then changing the local baud-rate may result in the queued output data
 * being sent at the wrong baud rate. This contravenes the POSIX 1003.1
 * definition of tcdrain() - it should not return until the implied output
 * is physically complete. The obvious alternative method - calling
 * tcsetattr() with the TCSADRAIN option - also suffers from the same
 * defect in the Linux kernel.
 *
 * Thus when changing the local baud-rate under Linux, the following
 * workround is needed:
 *
 *	(1) call tcdrain() to initiate transmission of queued output data.
 *	(2) sleep for a bit to let the drain complete.
 *	(3) change the local baud-rate.
 *	(4) call tcflush() with TCIFLUSH to discard garbled input.
 *
 * Unlike other serial-communications applications, slap will reliably
 * communicate with the printer at 38400 baud even through ports using
 * ancient 8250 UARTs. This is possible due to the extremely asymmetric
 * nature of the printer command protocol, and due to the fact that these
 * printers "pace" there own transmissions. Well, at least the SLP-2000
 * guarantees no more than one byte transmitted every 10 microseconds.
 * For those printers capable of 57600 baud, old 8250 UARTs (circa 1983-90)
 * are not reliable.
 *
 *****************************************************************************
 */

#define XON	0x11
#define XOFF	0x13


/* physical printer maximum imaging-size and imaging-margins, in millimetres:
 * Margins expressed relative to the printer, rather than relative to
 * the logical media...
 */
typedef struct
	{
	DOUBLE	top, bottom, left, right;
	} MARGINS;


typedef struct
	{
	PTYPE ptype;		/* must be unique */
	CHAR *name;		/* model-name */
	UTINY model_id;		/* from printer, as provoked by CMD_MODEL */
	UTINY maxwidth;		/* of label (excluding physical margins), in mm */
	MARGINS um;		/* unprintable physical margins, in mm */
	DOUBLE vpitch;
	DOUBLE hpitch;
	BOOL will_auto_offline;
	BOOL can_tune_xonxoff;
	BOOL has_slop_bug;
	BOOL does_rtscts;	/* printer does RTS/CTS flow-control
				 * (they all *always* perform XON/XOFF
				 * flow-control, even those that also do
				 * RTS/CTS...)
				 */
	BOOL is_faint;
	BOOL can_57600_baud;
	BOOL has_antibanding;
	LONG default_baud;	/* eg: after reset or power-on */
	} MODEL;

static const MODEL models[] =
	{
		{
		PT_SLP_PRO,
			"SLP-Pro", 0x2,
			54u, {4.0, 4.0, 1.0, 2.0}, 7.8, 8.0,
			NO, NO, NO, NO, YES, NO, NO, 9600
		},
		{
		PT_SLP_120,
			"SLP-120", 0x4,
			28u, {4.0, 4.0, 2.0, 1.0}, 8.0, 8.0,
			/* slp120 *definately* has the slop bug */
			YES, YES, YES, YES, NO, YES, YES, 9600
		},
		{
		PT_SLP_100,
			"SLP-100", 0x6,
			28u, {4.0, 4.0, 2.0, 1.0}, 8.0, 8.0,
			/* slp100n *probably* has the slop bug */
			YES, YES, YES, YES, NO, YES, YES, 57600
		},
		{
		PT_SLP_100N,
			"SLP-100N", 0x8,
			28u, {4.0, 4.0, 2.0, 1.0}, 8.0, 8.0,
			/* slp100n *probably* has the slop bug */
			YES, YES, YES, YES, NO, YES, YES, 57600
		},
		{
		PT_SLP_220,
			"SLP-220", 0x5,
			54u, {4.0, 4.0, 1.0, 2.0}, 8.0, 8.0,
			YES, YES, NO, YES, NO, YES, YES, 9600
		},
		{
		PT_SLP_200,
			"SLP-200", 0x7,
			54u, {4.0, 4.0, 1.0, 2.0}, 8.0, 8.0,
			YES, YES, NO, YES, NO, YES, YES, 57600
		},
		{
		PT_SLP_200N,
			"SLP-200N", 0x9,
			54u, {4.0, 4.0, 1.0, 2.0}, 8.0, 8.0,
			YES, YES, NO, YES, NO, YES, YES, 57600
		},
		{
		PT_SLP_240,
			"SLP-240", 0xB,
			54u, {4.0, 4.0, 1.0, 2.0}, 8.0, 8.0,
			YES, YES, NO, YES, NO, YES, YES, 57600
		},
		/* terminator entry: */
		{
		PT_NONE,
			"unknown", 0x0,
			0, {0.0, 0.0, 0.0, 0.0}, 0.0, 0.0,
			NO, NO, NO, NO, NO, NO, NO, B0
		}
	};


#define SLICEWIDTH		8u	/* horizontal dots per slice */
#define MAXNOPS			54	/* maximum number of CMD_NOPs needed
					 * to terminate pending partial
					 * print command, for any model */


/* driver printer status bits:
 */
#define STAT_PAPER_OUT	0x001	/* physical */
#define STAT_PAPER_JAM	0x002	/* physical */
#define STAT_HARD_ERR	0x004	/* physical */
#define STAT_COMM_ERR	0x008	/* physical: communication- or command-error */
#define STAT_IDLE	0x010	/* physical */
#define STAT_PLATEN_OPEN 0x020	/* physical: platen is open (not on SLP-2000) */
#define STAT_GARBLED	0x100	/* soft: baud-rate mismatch */
#define STAT_BUF_FULL	0x200	/* soft */
#define STAT_CHECKPOINT	0x400	/* soft */
#define STAT_PING_OK	0x800	/* soft */
 

/* mask of printer-status bits that are physically sent by the printer
 * (as distinct from "soft" status bits which are generated internally by
 * this driver):
 */
#define PHYS_STAT_MASK \
	(STAT_PAPER_OUT|STAT_PAPER_JAM|STAT_HARD_ERR|STAT_COMM_ERR|STAT_IDLE|STAT_PLATEN_OPEN)

 
/* a mask of printer-status bits that require a printer-reset:
 */
#define RESET_STAT_MASK (STAT_PAPER_JAM | STAT_HARD_ERR | STAT_COMM_ERR)


/* a mask of printer-status bits that indicate an error:
 */
#define ERROR_STAT_MASK (RESET_STAT_MASK | STAT_PAPER_OUT | STAT_GARBLED) 

 
typedef struct
	{
	BYTE *slices;
	} BAND;

/* structure holding private driver state-data:
 */
typedef struct
	{
	const CHAR *portname;	/* name of comms port, for error messages */
	INT fd;
	struct termios tm;	/* current termios */
	struct termios otm;	/* original termios */
	BOOL tof;		/* are we at the top-of-label? */
	TINY margin;		/* physical offset of left margin, in mm */
	BAND *map;		/* device-format image bitmap */
	USHORT nbands, nslices;	/* dimensions of map */
	LONG maxbaud;		/* per session limit for data-transfer phase */
	const MODEL *m;
	USHORT ps;		/* logical and physical printer status */
	} STATE;


/* forward declarations:
 */
static USHORT check_fault(STATE *st, INT timeout);
static BOOL wait_for_ready(STATE *st, INT timeout);
static USHORT sense(STATE *st, INT timeout);
static VOID set_baud(STATE *st, LONG);


static const MODEL *valid_ptype(PTYPE ptype)
	{
	const MODEL *p;

	for (p = models; p->ptype; ++p)
		{
		if (p->ptype == ptype)
			return (p);
		}
	return (NULL);
	}


#if DEBUG_LEVEL

/* decode abstract status-byte into human-readable form
 */
static const CHAR *statstr(USHORT s)
	{
	CHAR buf[BUFSIZ] = "";

	if (s & STAT_PAPER_JAM)
		{
		s &= ~ STAT_PAPER_JAM;
		strcat(buf, "|jam");
		}
	if (s & STAT_PAPER_OUT)
		{
		s &= ~ STAT_PAPER_OUT;
		strcat(buf, "|empty");
		}
	if (s & STAT_HARD_ERR)
		{
		s &= ~ STAT_HARD_ERR;
		strcat(buf, "|fault");
		}
	if (s & (STAT_COMM_ERR))
		{
		s &= ~ STAT_COMM_ERR;
		strcat(buf, "|protocol_error");
		}
	if (s & STAT_GARBLED)
		{
		s &= ~ STAT_GARBLED;
		strcat(buf, "|garbled");
		}
	if (s & STAT_IDLE)
		{
		s &= ~ STAT_IDLE;
		strcat(buf, "|idle");
		}
	if (s & STAT_PLATEN_OPEN)
		{
		s &= ~STAT_PLATEN_OPEN;
		strcat(buf, "|lid open");
		}
	if (s & STAT_BUF_FULL)
		{
		s &= ~STAT_BUF_FULL;
		strcat(buf, "|hold");
		}
	else
		strcat(buf, "|ready");
	if (s)
		strcat(buf, "|other");
	return (buf[0] ? &buf[1] : "none");
	}
#endif


/* receive bytes from the printer, with timeout.
 * timeout is expressed in milliseconds, to 0.1 second resolution.
 */
static INT recv_bytes(STATE *st, BYTE *pb, UINT maxlen, INT timeout)
	{
	static INT old_timeout = -49;	/* an unlikely initial value! */
	INT e;

	if (old_timeout != timeout)
		{
		if (timeout < 0)
			timeout = 0;

		/* want read() to return when any data available */
		st->tm.c_cc[VMIN] = 0;
		st->tm.c_cc[VTIME] = timeout;

		/* Note: is safe to use TCSANOW because we are not changing
		 * any output modes and are not changing those input modes that
		 * would effect data in transit...
		 */
		if (tcsetattr(st->fd, TCSANOW, &st->tm) < 0)
			{
			Error("cannot set read-timeout on serial-port: %s",
				st->portname);
			}
		old_timeout = timeout;
		}

	errno = 0;
	e = read(st->fd, pb, maxlen);
	if (e < 0)
		{
		Error("read error on %s", st->portname);
		e = 0;
		}
	return (e);
	}


/* send a sequence of bytes to the printer
 */
static BOOL send_bytes(STATE *st, BYTE *bytes, INT nbytes, BOOL blind)
	{
	if (!blind)
		{
		if (!wait_for_ready(st, 20))
			Error("timeout waiting for printer to become ready");
		}
	errno = 0;
	if (write(st->fd, bytes, nbytes) != nbytes)
		Error("write error on %s", st->portname);
	return (YES);
	}


/* send a single byte to the printer
 */
static BOOL send_byte(STATE *st, BYTE b, BOOL blind)
	{
	return (send_bytes(st, &b, 1, blind));
	}


/* sense the printer status.
 * The printer only sends an unsolicited status byte when it's status
 * actually CHANGES. Therefore, if the read() on the port times-out,
 * we assume that the last received status byte is still up-to-date...
 * NOTE: under some POSIX.1 implementations, you can read XON and XOFF
 * characters even when IXON and/or IXOFF are enabled - specifically
 * when non-paired XON or XOFF characters are received. We need to
 * ignore any such characters read...
 */
static USHORT sense(STATE *st, INT timeout)
	{
	static BYTE buf[BUFSIZ];
	UINT i, n;
	BYTE s, sb;
	const MODEL *p;

	st->ps &= ~(STAT_CHECKPOINT | STAT_PING_OK);
	sb = 0;
	while (YES)
		{
		if (!(n = recv_bytes(st, buf, BUFSIZ, timeout)))
			break;	/* no change: old value is still current */
		s = XON;
		for (i = 0; i < n; ++i)
			{
			s = buf[i] & 0xFF;
			switch (s)
				{
			case XON:
				if (st->ps & STAT_BUF_FULL)
					TRACE((3, "read XON"));
				st->ps &= ~STAT_BUF_FULL;
				tcflow(st->fd, TCOON); /* paranoia */
				break;
			case XOFF:
				if (!(st->ps & STAT_BUF_FULL))
					TRACE((3, "read XOFF"));
				st->ps |= STAT_BUF_FULL;
				tcflow(st->fd, TCOOFF);	/* paranoia */
				break;
			case 0xC7:
				timeout = 0;
				st->ps |= STAT_CHECKPOINT;
				st->ps &= ~STAT_GARBLED;
				break;
			case 0xC9:
				timeout = 0;
				st->ps |= STAT_PING_OK;
				st->ps &= ~STAT_GARBLED;
				break;
			default:
				st->ps &= ~STAT_GARBLED;
				switch (s & 0xF0)
					{
				case 0xD0: /* response to a CMD_GETOPTIONS */
					timeout = 0;
					break;
				case 0xE0: /* reponse to a CMD_MODEL */
					timeout = 0;
					st->m = NULL;
					for (p = models; p->ptype; ++p)
						{
						if ((s & ~0xE0) == p->model_id)
							st->m = p;
						}
					break;
				case 0x80: /* response to CMD_VERSION */
					timeout = 0;
					TRACE((4, "firmware version: %u",
						s & ~0x80));
					break;
				default:
					if ((s & 0xC0) == 0x40) /* status */
						{
						/* just remember it, because
						 * we are only interested in
						 * the final one received so
						 * far.
						 */
						timeout = 0;
						sb = s;
						}
					else
						{
						/* assume garbled due to
						 * wrong baud-rate
						 */
						TRACE((3, "%s: 0x%X",
							"recv wierd byte", s));
						st->ps |= STAT_GARBLED;
						}
					break;
					}
				}
			}

		/* now inspect final status-byte seen:
		 */
		if (sb & 0x40)
			{
			USHORT new;

			/* careful to preserve "logical" status-bits: */
			new = (st->ps & ~PHYS_STAT_MASK);
			sb &= ~0x40;
			if ((sb & PHYS_STAT_MASK) != sb)
				{
				TRACE((4, "wierd status: 0x%X", sb));
				new |= STAT_GARBLED;
				}
			else
				timeout = 0;
			new |= sb & PHYS_STAT_MASK;
			if (st->ps != new)
				TRACE((3, "status: %s", statstr(new)));
			st->ps = new;
			}
		}
	return (st->ps);
	}


/* terminate any partially-transmitted "print" commands...
 * This is needed to force the printer into a state where it will
 * accept commands.
 */
static VOID force(STATE *st)
	{
	const BYTE b = 0x0;
	UINT i;

	TRACE((3, "terminating data cycle"));

	/* cycle termination is done by sending "nop" commands until either:
	 *
	 * 	(1) we have sent enough to fill a print-data segment, or
	 *	(2) the printer reports STAT_IDLE or STAT_GARBLED.
	 *
	 * The test for STAT_IDLE is in case the printer is already not in a
	 * band-data cycle...
	 *
	 * This termination technique uses the fact that command "nops" are
	 * also "data" nops, and that the NOP bit-pattern should be
	 * non-invasive even at the wrong baud-rate.
	 */
	tcflush(st->fd, TCOFLUSH);	/* flush queued output */
	for (i = 0; i < (st->nslices + 2); ++i)
		{
		if (sense(st, 0) & (STAT_IDLE|STAT_GARBLED))
			{
			tcflush(st->fd, TCOFLUSH); /* flush queued NOPs */
			break;
			}
		/* enable transmit (sense() may have disabled it...) */
		tcflow(st->fd, TCOON);

		/* queue NOP for transmit */
		write(st->fd, &b, 1);
		tcdrain(st->fd);
		}
	tcflow(st->fd, TCOON);
	TRACE((3, "data cycle terminated: sent %i NOPs", i));
	}


/* detect and handle paper-out condition.
 */
static VOID check_loaded(STATE *st)
	{
	if (sense(st, 0) & STAT_PAPER_OUT)
		{
		do
			{
			Warning("printer empty: load labels");
			} while (sense(st, 97) & STAT_PAPER_OUT);

		sleep(2);	/* to enable the human to stand clear! */
		Notice("labels loaded");
		}
	}


/* detect and handle platen-open condition.
 */
static VOID check_closed(STATE *st)
	{
	if (sense(st, 0) & STAT_PLATEN_OPEN)
		{
		do
			{
			Warning("platen open: close lid");
			} while (sense(st, 91) & STAT_PLATEN_OPEN);

		sleep(2);	/* to enable the human to stand clear! */
		Notice("lid closed");
		}
	}


/* wait for printer to become idle, with timeout in seconds.
 * If printer is in an error-state that would prevent an "idle" status,
 * we attempt to clear the error and restart the timeout interval.
 * Returns YES if printer became idle, else NO.
 */
static BOOL wait_for_idle(STATE *st, INT timeout)
	{
	time_t t1;
 
	tcdrain(st->fd);
	t1 = time(NULL);
	do
		{
		if (sense(st, timeout) & STAT_IDLE)
			return (YES);
		if (st->ps & ERROR_STAT_MASK)
			{
			check_fault(st, 23);
			t1 = time(NULL);
			}
		} while ((INT)difftime(time(NULL), t1) < timeout);
 
	return (NO);	/* timed-out! */
	}
 

/* wait for printer to become ready to receive more commands/data, with
 * timeout in seconds.
 * If printer is in an error-state that would prevent a "ready" status,
 * we attempt to clear the error and restart the timeout interval.
 */
static BOOL wait_for_ready(STATE *st, INT timeout)
	{
	time_t t1;

	/* MJS: do we need a tcdrain() here? */

	t1 = time(NULL);
	do
		{
		if (!(sense(st, 0) & STAT_BUF_FULL))
			return (YES);
		if (st->ps & ERROR_STAT_MASK)
			{
			check_fault(st, 0);
			t1 = time(NULL);
			}
		} while ((INT)difftime(time(NULL), t1) < timeout);

	return (NO);	/* timed-out */
	}


static VOID set_density(STATE *st, DENSITY dens)
	{
	static BYTE code[5] = {0xFC, 0xFE, 0x00, 0x02, 0x04};
	BYTE b;

	b = code[dens];
	send_byte(st, 0xE, NO);
	send_byte(st, b, NO);
	}


static VOID set_margin(STATE *st)
	{
	TRACE((2, "set physical indent to %i mm", st->margin & 0xFF));
	send_byte(st, 0x6, YES);
	send_byte(st, st->margin & 0xFF, YES);
	}

/* match local baud-rate with the printer...assumes suitable vanilla
 * modes have already been set on the channel...
 * If successful, sets local baud-rate to match the printer and return YES.
 * Otherwise, returns NO (ie: printer not responding, or wrong type of printer).
 */
static BOOL ping(STATE *st)
	{
	/* NOTE: the speeds to try are arranged in general order of likelyhood
	 * across all supported printer-models, to minimise the time taken to
	 * complete the probe...
	 */
	static const LONG speeds[] = {9600, 57600, 38400, 19200, 0};
	INT i;

	for (i = 0; speeds[i] != 0; ++i)
		{
		if (val2baud(speeds[i]))
			{
			TRACE((3, "pinging at %lu baud", speeds[i]));
			tcflush(st->fd, TCIOFLUSH);
			set_baud(st, speeds[i]);
			force(st);

			send_byte(st, 0xA5, YES);	/* "ping" */

			/* wait upto 1 seconds for reply */
			sense(st, 10);
			if (st->ps & STAT_PING_OK)
				break;
			}
		}
	if (speeds[i] == 0)
		return (NO);

	/* request printer status with "immediate-mode" command, to
	 * verify the connection.
	 * NOTE: the SLP-2000 may not respond to status requests for upto
	 * 8 seconds immediately after being switched-on. This seems
	 * to depend on the printer's firmware revision.
	 * We will cater for that possibility...
	 */
	tcflush(st->fd, TCIFLUSH);
	st->ps = 0;
	send_byte(st, 0x1, YES);
	if (!wait_for_idle(st, 8))
		return (NO);

	return (YES);
	}


/* determine printer model, non-intrusively.
 * Assumes printer has been pinged successfully.
 * Sets the "m" field of the "state" structure.
 */
static const MODEL *getmodel(STATE *st)
	{
	/* send immediate-mode CMD_MODEL.
	 *
	 * The SLP-1X0 and SLP-2X0 will
	 * respond with a model-id byte; the SLP-2000 is supposed to respond
	 * with a "command-error" status-byte (according to Seiko), but in fact
	 * will just ignore the command completly...
	 *
	 * If we get no response, we can safely assume it is an SLP-2000,
	 * because we already know that it responds to the right kind of pings.
	 */
	st->m = NULL;
	send_byte(st, 0x12, YES);
	if ((sense(st, 5) & STAT_COMM_ERR) || !st->m)
		st->m = valid_ptype(PT_SLP_PRO);

	TRACE((2, "detected: %s", st->m->name));

	return (st->m);
	}


static VOID set_printer_modes(STATE *st)
	{
	if (st->m->will_auto_offline)
		{
		TRACE((6, "awakening printer"));
		send_byte(st, 0x1E, YES);	/* force online */
		send_byte(st, 0x0, YES);
		}

	if (st->m->is_faint)
		set_density(st, DENS_BOLD);
	else
		set_density(st, DENS_NORM);

	if (st->m->has_antibanding)
		{
		/* disable anti-banding, because it is only useful for large
		 * black printed areas on dodgy media, and it slows
		 * everything way down...
		 */
		send_byte(st, 0x17, YES);
		send_byte(st, 0x0, YES);
		}

	if (st->m->can_tune_xonxoff)
		{
		/* we want to normalise the printers' XON/XOFF thresholds,
		 * but these can only be changed if the printers' buffer
		 * is empty...
		 */
		if (wait_for_idle(st, 1))
			{
			send_byte(st, 0x18, YES); /* XOFF threshold is... */
			send_byte(st, 32, YES);	  /* 32 bytes (the default) */
			send_byte(st, 0x19, YES); /* XON threshold is... */
			send_byte(st, 100, YES);  /* 100 bytes (the default) */
			}
		}

	set_margin(st);
	}


/* switch the printer baud-rate and then the local to match
 */
static BOOL switch_baud(STATE *st, LONG speed)
	{
	tcflow(st->fd, TCOON);
	send_byte(st, 0x3, YES);
	tcdrain(st->fd);

	if ((st->maxbaud > 0) && (st->maxbaud < speed))
		speed = st->maxbaud;

	TRACE((3, "switching to %lu baud", speed));

	switch (speed)
		{
	default:
	case 9600:
		send_byte(st, 0, YES);
		break;
	case 19200:
		send_byte(st, 1, YES);
		break;
	case 38400:
		send_byte(st, 2, YES);
		break;
	case 57600:
		send_byte(st, 3, YES);
		break;
		}

	/* must not change our local baud-rate until the queued
	 * output has drained, otherwise it may get transmitted
	 * at the wrong baud-rate!
	 *
	 * According to POSIX.1, tcdrain() will not return until
	 * all queued output has been transmitted. Unfortunately,
	 * under Linux, tcdrain() only initiates such transmission -
	 * it does not wait for it to complete (yuk)!
	 * So under Linux we must explicitly wait a while.
	 */
	tcdrain(st->fd);
	#if defined(__linux__) || (TCDRAIN_BROKEN != 0)
	sleep(1);
	#endif

	set_baud(st, speed);

	/* and we must not transmit anything else for 200ms, otherwise
	 * the printer will get wedged...
	 */
	if (wait_for_idle(st, 1))
		TRACE((6, "printer idle after baud-rate change"));
	return (YES);
	}


/* reset the printer and wait for "ready" status.
 * Returns printer status.
 * Because this may be called at unpredictable times, we must ensure that
 * the printer is forced into command-mode first.
 */
static USHORT reset_printer(STATE *st)
	{
	TRACE((3, "resetting printer..."));

	/* normalise printer and comms channel:
	 */
	force(st);
	if (!ping(st) || !getmodel(st))
		Error("printer not responding");
	tcflush(st->fd, TCIOFLUSH);

	/* request hard reset:
	 */
	st->ps = 0;
	send_byte(st, 0xF, YES);
	tcdrain(st->fd);

	if ((st->m->default_baud == 57600) && !val2baud(57600))
		Error("57600 baud not enabled on this system");
	set_baud(st, st->m->default_baud);

	tcflush(st->fd, TCIFLUSH);	/* ...maybe not needed! */
	if (!wait_for_idle(st, 5))
		Warning("printer not responding to reset requests");

	return (st->ps);
	}


/* set the local baud-rate on the comms channel.
 * This discards all pending input.
 */
static VOID set_baud(STATE *st, LONG speed)
	{
	speed_t b = val2baud(speed);

	TRACE((4, "setting local baud rate with token 0x%lx", (long) b));
	
	/* NOTE: the POSIX 1003.1 spec says that it is implementation-dependant
	 * whether cfsetispeed(), cfsetospeed() and tcsetattr() will indicate
	 * failure when the requested baud-rate is unattainable on the implied
	 * device. So there are circumstances when this lot could fail SILENTLY,
	 * depending on the sensibilities of your Operating-System vendor.
	 */
	if ((cfsetospeed(&st->tm, b) < 0) ||
		(cfsetispeed(&st->tm, b) < 0) ||
		(tcsetattr(st->fd, TCSAFLUSH, &st->tm) < 0))
		{
		Error("cannot set baud-rate on serial-port: %s", st->portname);
		}

	tcflush(st->fd, TCIFLUSH);	/* paranoia! */
	}


static VOID formfeed(STATE *st, BOOL blind)
	{
	TRACE((3, "feed page"));
	send_byte(st, 0xC, blind);
	send_byte(st, 0x1, blind);
	st->tof = YES;
	if (!blind)
		{
		tcdrain(st->fd);
		if (!wait_for_idle(st, 20))
			Error("timeout waiting for printer to become idle");
		}
	}


/* Detect and handle printer fault.
 */
static USHORT check_fault(STATE *st, INT timeout)
	{
	sense(st, timeout);

	if (st->ps & RESET_STAT_MASK)
		{
		USHORT s = st->ps;

		/* try to force printer into sane state for the next run...
		 */
		reset_printer(st);
		if (!st->tof)
			formfeed(st, NO);

		/* prepare for fast shutdown...
		 */
		closetty(st->fd, &st->otm);
		st->fd = -1;

		/* report error and abort...
		 */
		if (s & STAT_HARD_ERR)
			Error("hardware fault");
		else if (s & STAT_COMM_ERR)
			Error("communication error");
		else  /* if (s & STAT_PAPER_JAM) */
			Error("label jammed in printer");
		}
	check_closed(st);
	check_loaded(st);
	return (st->ps);
	}


/* Initialise a communications-channel from an open file-descriptor.
 * Sets the (initial) local serial-port attributes and modes.
 */
static VOID init_channel(STATE *st)
	{
	/* NOTE: assume that we have already tested st->fd for ENOTTY here */
	tcgetattr(st->fd, &st->tm);


	/* select "output" modes (completely raw)...
	 */
	st->tm.c_oflag &= ~OPOST;

	/* select "physical" input modes (start completely raw)...
	 */
	st->tm.c_iflag &= ~(IGNBRK|IGNPAR|PARMRK|INPCK|ISTRIP);
	st->tm.c_iflag &= ~(INLCR|IGNCR|ICRNL);
	st->tm.c_iflag &= ~(IXOFF);
	st->tm.c_iflag &= ~BRKINT;

	/* set "physical" pseudo-output-modes
	 * Enable XON/XOFF transmit flow-control.
	 */
	st->tm.c_cc[VSTOP] = XOFF;	
	st->tm.c_cc[VSTART] = XON;
	st->tm.c_iflag |= IXON;

	/* select "logical" input modes (completely raw)...
	 */
	st->tm.c_lflag &= ~(ECHO|ECHOE|ECHOK|ECHONL);
	st->tm.c_lflag &= ~(ISIG|ICANON|IEXTEN);
	st->tm.c_lflag &= ~(TOSTOP);
	st->tm.c_lflag |= NOFLSH;

	/* additional "logical" input modes...
	 * Want arbitrary-size read() responses, with timeout.
	 * The values used in c_cc for this are magic-cookies - read the
	 * POSIX 1003.1 spec for the gory details.
	 * NOTE: the timeout value will be changed by the program as required.
	 */
	/* read() returns as soon as data is available: */
	st->tm.c_cc[VMIN] = 0;
	/* read() times-out after 0.1 seconds: */
	st->tm.c_cc[VTIME] = 1;

	/* select "bit-framing" (8n1):
	 */
	st->tm.c_cflag &= ~(CSIZE);
	st->tm.c_cflag |= CS8;

	/* select "control" modes:
	 * (ignore DCD, enable receive, no parity, and
	 * drop DTR when we close the port)...
	 */
	st->tm.c_cflag &= ~(CSTOPB|PARENB|PARODD);
	st->tm.c_cflag |= CLOCAL|CREAD|HUPCL;

	/* apply the modes selected above:
	 * note the use of TCSAFLUSH, in case the serial-port driver has
	 * not physically completed any outstanding (buffered) output...
	 */
	if (tcsetattr(st->fd, TCSAFLUSH, &st->tm) < 0)
		Error("cannot set tty modes on serial-port: %s", st->portname);

	/* and discard any unread input, which would be garbled anyway...
	 */
	tcflush(st->fd, TCIFLUSH);
	}


static VOID printlabel(OPD *opd, const ARENA *a)
	{
	STATE *st = opd->private;
	UINT c, n, band, slice = 0, lastband = 0;

	/* convert MBM-format bitmap into "device" bitmap
	 * (an array of 1-pixel-high slices, one MSB-first slice for each
	 * horizontal position of the print-head)...
	 */
	TRACE((3, "mapping bitmap"));
	assert(st->map);
	assert((st->nslices * SLICEWIDTH) == mbm_width(a->bm));
	assert(st->nbands == mbm_height(a->bm));

	for (band = 0; band < st->nbands; ++band)
		{
		BAND *b = &st->map[band];

		for (slice = 0; slice < st->nslices; ++slice)
			{
			b->slices[slice] = 0;
			for (n = 0; n < SLICEWIDTH; ++n)
				{
				c = (slice * SLICEWIDTH) + n;
				#define SLOP 1
				if ((c < ((st->nslices * SLICEWIDTH) - SLOP)) &&
						(st->m->has_slop_bug))
					{
					/* the SLP-120 etc have a firmware
					 * bug: when the horizontal
					 * 8-dot print-head is in it's
					 * rightmost possible position,
					 * the rightmost dot does not
 					 * fire, which results in the
					 * rightmost strip of pixels
					 * not printing. We fudge it
					 * by changing the mapping one
					 * pixel to the left, which
					 * drops the leftmost (bottom)
					 * strip of pixels, which is
					 * usually empty.
					 * SPOOF: I am assuming that the
					 * SLP-100 and SLP-100N also have
					 * this problem, until proven otherwise.
					 */
					c += SLOP;
					}
				if (mbm_tstb(a->bm, band, c))
					{
					b->slices[slice] |= (0x80 >> n);
					lastband = band;
					}
				}
			}
		}

	/* now print it!
	 */
	TRACE((3, "printing bitmap: %u(%u) x %u", band, lastband, slice));

	/* feed three blank bands to adjust for mistensioned media;
	 * also forces us back to the left margin...and also forces
	 * printer to check for an "empty" condition!
	 */
	for (n = 0; n < 3; ++n)
		{
		send_byte(st, 0xA, NO);
		st->tof = NO;
		tcdrain(st->fd);
		check_fault(st, 2);
		}

	for (band = 0; band <= lastband; ++band)
		{
		st->ps &= ~STAT_IDLE;
		send_byte(st, 0x4, NO);
		send_byte(st, st->nslices, NO);
		/* NOTE: we are relying on the flow-control here! */
		/* ...should this be asynchronous? */
		send_bytes(st, st->map[band].slices, st->nslices, NO);
		check_fault(st, 0);
		}

	wait_for_idle(st, 8);
	formfeed(st, NO);
	}


/* connect to printer, and determine exact model
 * NOTE: a target of "-" means use standard-output for comms.
 */
static PTYPE connect(STATE *st, const CHAR *target)
	{
	PTYPE ptype = PT_NONE;
	const CHAR *portname;
	BOOL locking = YES;

	TRACE((3, "connecting (SLP-Pro family driver)"));
	assert(st && (st->fd < 0) && !st->portname && target);

	st->ps = 0;
	/* set st->portname to full name of target, for any error-messages */
	if (!strcmp(target, "-"))
		{
		if (!(portname = ttyname(STDOUT_FILENO)))
			portname = "standard-output";
		locking = NO;
		}
	else
		portname = target;

	st->portname = str_dup(portname, YES);

	if ((st->fd = opentty(target, O_RDWR|O_NOCTTY, &st->otm)) < 0)
		Error("cannot open serial-port: %s", st->portname);

	if (!isatty(st->fd))
		Error("%s is not a serial-port", st->portname);
	if (locking && !fxlock(st->fd, 10))
		Error("serial-port %s is already in use", st->portname);

	/* set serial-port modes and setup printer for printing...
	 */
	init_channel(st);
	if (!ping(st) || !getmodel(st))
		return (ptype);

	/* now we have finished probing and know we are connected to
	 * an appropriate printer for this driver, it is safe to
	 * enable signal-on-serial-break semantics:
	 * We want to be sent a SIGINT if we get sent bytes without
	 * valid start or stop bits (interference on cable, cable
	 * unplugged, etc).
	 * Actually will only get the signal if the port is the "controlling
	 * terminal" of our (foreground) process-group. Some implementations
	 * of lp or lpr may arrange for this to be true, others may not. If
	 * the signal is received, the session-handling framework will
	 * handle it.
	 */
	st->tm.c_iflag |= BRKINT;
	tcsetattr(st->fd, TCSANOW, &st->tm);

	/* if both the host and the printer support 57600 baud *and*
	 * RTS/CTS flow-control, we take advantage of the fast comms
	 * capability...
	 * Otherwise we fall back to 38400 baud using XON/XOFF flow-control...
	 */
	if (val2baud(57600) &&
			st->m->can_57600_baud &&
			st->m->does_rtscts && use_cts(st->fd))
		{
		TRACE((4, "enabled hardware flow-control"));
		switch_baud(st, 57600);
		}
	else
		switch_baud(st, 38400);

	set_printer_modes(st);
	wait_for_idle(st, 2);

	TRACE((3, "connected"));
	return (st->m->ptype);
	}

static VOID disconnect(STATE *st)
	{
	TRACE((3, "disconnecting"));

	if (st)
		{
		if (st->fd >= 0)
			{
			avoid_cts(st->fd);
			tcflow(st->fd, TCOON);
			tcdrain(st->fd);	/* just in case! */
			force(st);
			if (st->ps & (STAT_GARBLED | RESET_STAT_MASK))
				;
			else
				{
				if (!st->tof)
					formfeed(st, YES);

				/* wait upto 8 seconds for the printer to become
				 * idle...and switch printer back to 9600 baud.
				 */
				tcflow(st->fd, TCOON);
				st->ps &= ~STAT_IDLE;
				send_byte(st, 0x1, YES);
				wait_for_idle(st, 8);
				switch_baud(st, st->m->default_baud);
				}
			closetty(st->fd, &st->otm);
			st->fd = -1;
			}
		else
			{
			st->fd = -1;
			TRACE((3, "shortcut disconnect"));
			}
		st->portname = mem_free((VOID *) st->portname);
		}
	TRACE((3, "disconnected"));
	}


static BOOL geometry(PTYPE ptype, const MEDIA *m, GEOMETRY *g)
	{
	USHORT k = 0;
	const MODEL *model;
 
	assert((model = valid_ptype(ptype)));

	if (m->width > model->maxwidth)
		return(NO);

	/* compute geometry in pixels:
	 */
	g->top = model->um.top * model->vpitch;
	g->bottom = model->um.bottom * model->vpitch;
	g->right = model->um.right * model->hpitch;
	g->left = model->um.left * model->hpitch;

	g->height = (m->height * model->vpitch) - g->top - g->bottom;
	g->width = (m->width * model->hpitch) - g->left - g->right;

	/* ensure integral number of slices: */
	k = g->width % SLICEWIDTH;
	TRACE((3, "w=%i-%i, h=%i", g->width, k, g->height));
	g->right += k;
	g->width -= k;

	assert((g->width % SLICEWIDTH) == 0); 
	return (YES);
	}

static DOUBLE vres(OPD *opd)
	{
	const MODEL *model;

	assert((model = valid_ptype(opd->ptype)));
	return (model->vpitch);
	}

static DOUBLE hres(OPD *opd)
	{
	const MODEL *model;

	assert((model = valid_ptype(opd->ptype)));
	return (model->hpitch);
	}

/* start the driver for a new session
 */
static OPD *opendev(const CHAR *target, LONG maxbaudval,
	PTYPE tentative_ptype, DRV *chain)
	{
	OPD *opd;
	STATE *st = NULL;

	opd = mem_buy(NULL, sizeof(*opd), YES);
	opd->ptype = tentative_ptype;	/* unless proven otherwise */
	opd->private = NULL;

	if (target)
		{
		st = mem_buy(NULL, sizeof(*st), YES); 
		st->m = NULL;
		st->maxbaud = (maxbaudval > 0) ? maxbaudval : 0;
		st->fd = -1;
		st->portname = NULL;
		st->ps = 0;	/* until proven otherwise */
		st->tof = YES; 
		st->map = NULL;
		st->nslices = MAXNOPS;	/* until proven otherwise */
	
		opd->private = st;
		if (!(opd->ptype = connect(st, target)))
			{
			disconnect(st);
			mem_free(st);
			opd = mem_free(opd);
			}
		else
			opd->target = str_dup(target, YES);
		}
	else
		opd->target = NULL;

	return (opd);
	}

static BOOL setmedia(OPD *opd, const MEDIA *media, BOOL portrait,
	ARENA *a, GEOMETRY *g)
	{
	USHORT band, h, w;
	GEOMETRY gg;
	STATE *st = opd->private;

	if (!(geometry(opd->ptype, media, &gg)))
		return (NO);
	if (g)
		*g = gg;
	if (a)
		{
		a->bm = mbm_buy(gg.height, gg.width, YES);
		a->dirty = NO;
		}

	if (opd->target)
		{
		/* compute device physical margin, in millimetres:
		 */
		h = gg.height + gg.top + gg.bottom;
		w = gg.width + gg.left + gg.right;
		st->margin = st->m->maxwidth - ((DOUBLE)w / st->m->hpitch);
		st->margin >>= 1;
		/* correct for physical (mechanical) margin: */
		st->margin -= st->m->um.left;
		if (st->margin < 0)
			st->margin = 0;
		set_margin(st);

		/* allocate linear device-format bitmap:
		 */
		if (st->map)
			{
			for (band = 0; band < st->nbands; ++band)
				mem_free(st->map[band].slices);
			}
		st->nbands = gg.height;
		st->nslices = gg.width / SLICEWIDTH;
		assert(!(gg.width % SLICEWIDTH));
		TRACE((4, "nslices=%u, nbands=%u", st->nslices, st->nbands));
		st->map = mem_buy(st->map, st->nbands * sizeof(BAND), YES);
		for (band = 0; band < st->nbands; ++band)
			{
			st->map[band].slices =
				mem_buy(NULL, st->nslices, YES);
			memset(st->map[band].slices, 0, st->nslices);
			}
		}

	return (YES);
	}

/* close down the display-driver
 * NOTE: could be called asynchronously
 */
static VOID closedev(OPD *opd)
	{
	STATE *st = opd->private;

	if (opd->target)
		{
		disconnect(st);
		if (st->map)
			{
			USHORT band;

			for (band = 0; band < st->nbands; ++band)
				{
				if (st->map[band].slices)
					mem_free(st->map[band].slices);
				else
					break;
				}
			st->map = mem_free(st->map);
			}
		mem_free(st);
		mem_free(opd->target);
		}
	mem_free(opd);
	}


/***************************************************************************/

const DRV slpro_drv =
	{
	opendev,
	setmedia,
	printlabel,
	closedev,
	hres,
	vres
	};

