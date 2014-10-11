/* slap: UNIX SmartLabel printing program, version 2
 * Module: display-driver and model-driver for SLP-1x00 series printers
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
 * Core API:	ANSI X3.159-1989 C Language
 * Other APIs:	MJS Portable C Library
 *		IEEE Std 1003.1-1990 (aka POSIX.1)
 ***********************************************************************
 *
 * This driver supports the following Seiko SmartLabel printers:
 *
 *		Model No.	Product Name
 *		--------------------------------
 *		1000		SmartLabel
 *		1000P		SmartLabel-Plus
 *		1100		SmartLabel-EZ30
 *
 *
 * NOTE:
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
 * Note: the SLP-1x00 comes with an RJ11/DB25 null-modem cable, thus
 * as far as we are concerned, the printer is a DCE device.
 *
 ***********************************************************************
 */
static const char * const module = "@(#)" __FILE__ " 2.7 30jan2001 MJS";

#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include "mjsu.h"
#include "slap.h"
#include "port.h"


/* The SLP-1x00 must be connected to an RS-232 serial port.
 * The SLP has a RJ11 port which has a null-modem RJ11/DB25 cable fitted.
 * This combination acts as a DCE (a modem!). The SLP-1x00 series
 * support the following RS-232 signals:
 *
 *	TX	transmitted data	(relative to DTE)
 *	RX	received data		(relative to DTE)
 *	CTS	clear-to-send		(ie: DTE ready)
 *	GND	reference ground voltage
 *
 * CTS is only available if the optional "super" Seiko cable is used.
 * Flow-control is unidirectional (the printer assumes that it always OK
 * to transmit to the computer). Flow-control of the computer by the printer
 * is done using a proprietary busy-status-bit protocol.
 *
 *****************************************************************************
 *
 * The various status bits returned by the printer come in two distinct
 * flavours: exceptions and progress-reports. Exceptions are asynchronous
 * protocol events, progress-reports are synchronous (ie: you know when to
 * expect them, they occur in a predictable sequence at a predictable point
 * in the conversation).
 *
 * The only progress-reports available are:
 *
 *	STAT_IDLE	indicates that the printer has completed ALL
 *			commands so far sent.
 *	STAT_ACK	indicates that the printer has received a complete
 *			"print-left" or "print-right" command (this is not
 *			all that useful!).
 *
 * Apart from the usual (paper-out, comms-error, etc), there is a key
 * exception that can be reported:
 *
 *	STAT_BUF_FULL	indicates that the printer's input-buffer is almost
 *			full. This is used for flow-control.
 *
 * A whole sequence of commands can be sent (subject to flow-control)
 * and STAT_IDLE will only be signalled when all of them are complete.
 *
 * The printer only sends an unsolicited status-byte when it's status
 * actually changes (except that clearing STAT_ACK is not voluntarily
 * reported; STAT_ACK is not really a true "status" indicator).
 *
 * However, the printer checks for the presence of labels
 * ONLY upon receipt of one of the "feed" commands (or when switched on),
 * rather than all the time. Even an explicit "get status" command does
 * not in itself cause the printer to check that labels are loaded.
 * Thus it is only possible to test whether labels are loaded
 * in a painfully restricted set of situations.
 *
 *****************************************************************************
 *
 * Serial-Port (termios) Modes:
 *
 * Because we cannot assume that CTS from the printer is available, we
 * must operate the port in CLOCAL mode (we will rely on the busy-bit protocol
 * for flow control).
 *
 * The SLP-1x00 assumes transparent byte-wide i/o, so we must disable
 * many of the common default serial-driver translations (echo, cr/nl,
 * parity-marking, lower/upper case, printable representation, strip-to-7-bit,
 * and all that stuff).
 *
 * Because the SLP-1x00 serial-port does not provide any of the lines used for
 * "syncronous" (external clocking) RS-232 modes, the computer's port must
 * be programmed to use it's internal transmit-clock - in the RS-232 sense,
 * all communication is asyncronous.
 *
 * All members of the SLP-1x00 series communicate at a fixed baud-rate.
 * Early revisions of the SLP-1000 communicate at 19200 baud. All other
 * members of the family communicate at 9600 baud.
 *
 * The SLP-1x00 serial port operates with a fixed framing (1 start bit,
 * 8 data bits, 1 stop bits; no parity).
 *
 *****************************************************************************
 *
 * Platform Notes:
 *
 * Under PC-Linux (kernel versions upto 2.x), the standard serial-port
 * driver does not handle serial-ports with 8250 UARTs very well - the
 * program may run very slowly with such hardware under Linux.
 * The serial-port used should be equipped with a 16450 or better UART
 * (or a fixed serial-driver?).
 *
 *****************************************************************************
 */

#define min(a, b)	((a) < (b) ? (a) : (b))
#define max(a, b)	((a) > (b) ? (a) : (b))


typedef struct
	{
	BYTE *slices;
	USHORT min, max;	/* edges of "dirty" zone */
	} BAND;


/* structure holding private driver state-data:
 */
typedef struct
	{
	const CHAR *portname;	/* name of comms port, for error messages */
	INT fd;
	struct termios tm;	/* current termios */
	struct termios otm;	/* original termios */
	BOOL tof;	/* are we at the top-of-label? */
	BAND *map;	/* device-format image bitmap */
	USHORT nbands, nslices;	/* dimensions of map */
	BYTE ps;	/* printer status */
	} STATE;


/* physical printer maximum imaging-size and imaging-margins, in millimetres:
 * Margins expressed relative to the printer, rather than relative to
 * the logical media...
 */
#define BAND_HEIGHT	8u	/* vertical dots per pass of the print-head */
#define INTERLEAVE	2u	/* physical bands per logical band */
#define VPITCH			6.30	/* dots-per-millimetre */
#define HPITCH			3.86	/* dots-per-millimetre */
#define PHYS_MAX_WIDTH		28.0
#define PHYS_TOP_MARGIN		(4.0 + (1.0 * (BAND_HEIGHT / VPITCH)))
#define PHYS_BOTTOM_MARGIN	(4.0 + (1.0 * (BAND_HEIGHT / VPITCH)))
#define PHYS_LEFT_MARGIN	(0.0 + (2.0 / HPITCH))
#define PHYS_RIGHT_MARGIN	(0.0 + (2.0 / HPITCH))

/* the maximum minimum number of CMD_NOPs needed to terminate any
 * partially-transmitted print commands:
 */
#define MAXNOPS			((USHORT) HPITCH * PHYS_MAX_WIDTH)



/* driver printer status bits:
 */
#define STAT_PAPER_OUT	0x01
#define STAT_PAPER_JAM	0x02
#define STAT_HARD_ERR	0x04
#define STAT_COMM_ERR	0x08	/* communication-error */
#define STAT_CMD_ERR	0x10	/* command-error */
#define STAT_IDLE	0x20
#define STAT_BUF_FULL	0x40
#define STAT_ACK	0x80	/* band data received ok */
 

/* a mask of printer-status bits that require a printer-reset:
 */
#define RESET_STAT_MASK (STAT_PAPER_JAM | STAT_HARD_ERR | \
				STAT_COMM_ERR | STAT_CMD_ERR)


/* a mask of printer-status bits that indicate an error:
 */
#define ERROR_STAT_MASK (RESET_STAT_MASK | STAT_PAPER_OUT) 

 

/* forward declarations:
 */
static BYTE check_fault(STATE *st, INT timeout);
static BOOL wait_for_ready(STATE *st, INT timeout);
static BYTE sense(STATE *st, INT timeout);
static VOID set_baud(STATE *st, INT brc);


#if DEBUG_LEVEL

/* decode abstract status-byte into human-readable form
 */
static const CHAR *statstr(BYTE s)
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
		strcat(buf, "|comms_error");
		}
	if (s & STAT_CMD_ERR)
		{
		s &= ~ STAT_CMD_ERR;
		strcat(buf, "|cmd_error");
		}
	if (s & STAT_IDLE)
		{
		s &= ~ STAT_IDLE;
		strcat(buf, "|idle");
		}
	if (s & STAT_BUF_FULL)
		{
		s &= ~ STAT_BUF_FULL;
		strcat(buf, "|hold");
		}
	else
		strcat(buf, "|ready");
	if (s & STAT_ACK)
		s &= ~ STAT_ACK;
	if (s)
		strcat(buf, "|other");
	return (buf[0] ? &buf[1] : "none");
	}
#endif


/* receive bytes from the printer, with timeout.
 * timeout is expressed in tenths of a second.
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


/* send byte to the printer.
 */
static BOOL send_byte(STATE *st, BYTE b, BOOL blind)
	{
	if (!blind)
		{
		if (!wait_for_ready(st, 20))
			Error("timeout waiting for printer to become ready");
		}
	errno = 0;
	if (write(st->fd, &b, 1) != 1)
		Error("write error on %s", st->portname);
	return (YES);
	}

/* sense the printer status.
 * The printer only sends an unsolicited status byte when it's status
 * actually CHANGES. Therefore, if the read() on the port times-out,
 * we assume that the last received status byte is still up-to-date...
 * NOTE: because the printer clears STAT_ACK immediately after
 * indicating it, we need to explicitly accumulate it per read()...
 */
static BYTE sense(STATE *st, INT timeout)
	{
	static BYTE buf[BUFSIZ];
	UINT i, n;

	st->ps &= ~STAT_ACK;
	while (YES)
		{
		if (!(n = recv_bytes(st, buf, BUFSIZ, timeout)))
			break;	/* no change: old value is still current */
		else
			timeout = 0;
		for (i = 0; i < n; ++i)
			{
			if (buf[i] & STAT_ACK)
				{
				buf[n-1] |= STAT_ACK;
				break;
				}
			}
		if ((buf[--n] & ~STAT_ACK) != (st->ps & ~STAT_ACK))
			TRACE((3, "status: %s", statstr(buf[n])));
		st->ps = buf[n];
		if (st->ps & STAT_BUF_FULL)	
			tcflow(st->fd, TCOOFF);
		else
			tcflow(st->fd, TCOON);
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
	 *	(2) the printer reports STAT_ACK (end of band data), or
	 *	(3) the printer reports STAT_IDLE.
	 *
	 * The test for STAT_IDLE is in case the printer is already not in a
	 * band-data cycle...
	 *
	 * This termination technique uses the fact that command "nops" are
	 * also "data" nops, and that the NOP bit-pattern should be
	 * non-invasive even at the wrong baud-rate.
	 */
	tcflush(st->fd, TCOFLUSH);	/* flush queued output */
	for (i = 0; i < (st->nslices+2); ++i)
		{
		if (sense(st, 0) & (STAT_IDLE|STAT_ACK))
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
			check_loaded(st);
			t1 = time(NULL);
			}
		} while ((INT)difftime(time(NULL), t1) < timeout);

	return (NO);	/* timed-out! */
	}
	

/* wait for printer to become ready for more commands/data, with timeout in
 * seconds.
 * If printer is in an error-state that would prevent a "ready" status,
 * we attempt to clear the error and restart the timeout interval.
 */
static BOOL wait_for_ready(STATE *st, INT timeout)
	{
	time_t t1;

	tcdrain(st->fd);	/* Werner Panocha: push pending data before
				 * we try to sense status (avoids a termios
				 * race-condition that was hiding here - MJS).
				 */

	t1 = time(NULL);
	do
		{
		if (!(sense(st, 0) & STAT_BUF_FULL))
			return (YES);
		if (st->ps & ERROR_STAT_MASK)
			{
			check_fault(st, 0);
			check_loaded(st);
			t1 = time(NULL);
			}
		} while ((INT)difftime(time(NULL), t1) < timeout);

	return (NO);	/* timed-out */
	}


static VOID set_density(STATE *st, DENSITY dens)
	{
	static BYTE code[5] = {0xC4, 0xC2, 0x80, 0x82, 0x84};
	BYTE b;

	TRACE((3, "set density"));
	b = code[dens];
	send_byte(st, 0xE, NO);
	send_byte(st, b, NO);
	}


/* reset the printer and wait for "ready" status.
 * Returns printer status.
 * Because this may be called at unpredictable times, we must ensure that
 * the printer is forced into command-mode first.
 */
static BYTE reset_printer(STATE *st)
	{
	TRACE((3, "resetting printer..."));

	/* normalise printer and comms channel:
	 */
	force(st);
	tcflush(st->fd, TCIOFLUSH);

	/* request hard reset:
	 */
	st->ps = 0;
	send_byte(st, 0xF, YES);
	tcdrain(st->fd);
	tcflush(st->fd, TCIFLUSH);	/* wierd, but needed! */
	if (!wait_for_idle(st, 5))
		Warning("printer not responding to reset requests");

	return (st->ps);
	}


/* probe printer for baud-rate...assumes suitable vanilla modes have
 * already been set on the session...
 * Sets local baud-rate to match the printer.
 */
static PTYPE ping(STATE *st)
	{
	PTYPE ptype = PT_NONE;
	static speed_t bauds[] = {B9600, B19200, B0};
	BYTE buf[BUFSIZ];
	INT n, i, k;

	for (i = 0; bauds[i] != B0; ++i)
		{
		TRACE((3, "pinging at %s baud", baud2name(bauds[i])));
		tcflush(st->fd, TCIOFLUSH);
		set_baud(st, bauds[i]);
		force(st);

		send_byte(st, 0x88, YES);	/* "ping" */

		/* wait upto 0.3 seconds for reply */
		n = recv_bytes(st, buf, BUFSIZ, 3);
		for (k = 0; k < n; ++k)
			{
			if (buf[k] == 0x77)	/* a ping response */
				break;
			}
		if (k < n)
			break;
		}
	if (bauds[i] == B0)
		{
		/* no ping response, could be an old original SLP-1000
		 * (fixed speed 19200, not pingable).
		 */
		set_baud(st, B19200);
		}

	/* if we can request the printer firmware version
	 * with an immediate-mode CMD_VERSION, then it must be
	 * an SLP-1100. The older models will respond with a
	 * status-byte with STAT_CMD_ERR set...
	 */
	send_byte(st, 0x02, YES);
	/* wait upto 0.2 seconds for reply */
	n = recv_bytes(st, buf, BUFSIZ, 2);
	for (k = 0; k < n; ++k)
		{
		if (buf[k] & STAT_CMD_ERR)
			{
			if (bauds[i] != B0)
				ptype = PT_SLP_PLUS;
			else
				ptype = PT_SLP;
			break;
			}
		else if ((buf[k] & 0x20) == 0x20)
			{
			ptype = PT_SLP_EZ30;
			break;
			}
		}
	if (k >= n)
		return (PT_NONE);

	TRACE((2, "detected: %s",
		(ptype == PT_SLP_EZ30) ? "SLP-EZ30" :
		(ptype == PT_SLP_PLUS) ? "SLP-Plus" :
					"SLP-Classic"));

	/* request printer status...
	 * Due to a firmware bug in some members of the SLP-1000 family,
	 * the printer will completely ignore "get status" requests if:
	 *
	 *	the printer is out of labels, and
	 *	STAT_PAPER_OUT has already been reported
	 *	(maybe during a previous comms session!)
	 *
	 * The printer will still provide asyncronous status indications
	 * if it's internal status changes - so we can force a status
	 * indication by deliberately sending garbled serial data, such
	 * as a serial break (misframed bytes with all bits zero). This
	 * causes the printer to asynchronously send a full status
	 * byte with the STAT_CMD_ERR bit set due to the deliberate
	 * communication error!
	 *
 	 * IMPORTANT:	a serial "BREAK" condition is likely to mess up the
	 * 		autoprobing done by other drivers, so we must only do
	 *		it if we are certain that we are talking to a member
	 *		of the SLP-1X00 family.
	 *
	 * Here we will try a CMD_STATUS first, and resort to a serial-break
	 * if we get no response...
	 */
	st->ps = STAT_ACK;
	send_byte(st, 0x1, YES);
	if (sense(st, 10) & STAT_ACK)	/* no response */
		{
		/* forcibly get status */
		TRACE((4, "generating serial break (0x%X)", st->ps));
		tcsendbreak(st->fd, 0);	
		if (sense(st, 10) & STAT_CMD_ERR)
			st->ps &= ~STAT_CMD_ERR;
		else
			return (PT_NONE);
		}

	if (!wait_for_idle(st, 1))
		return (PT_NONE);

	return (ptype);
	}


/* set the local baud-rate on the comms channel.
 * This discards all pending input.
 */
static VOID set_baud(STATE *st, INT brc)
	{
	UINT i;

	/* NOTE: the POSIX 1003.1 spec says that it is implementation-dependant
	 * whether cfsetispeed(), cfsetospeed() and tcsetattr() will indicate
	 * failure when the requested baud-rate is unattainable on the implied
	 * device. So there are circumstances when this lot could fail SILENTLY,
	 * depending on the sensibilities of your Operating-System vendor.
	 */
	if ((cfsetospeed(&st->tm, brc) < 0) ||
		(cfsetispeed(&st->tm, brc) < 0) ||
		(tcsetattr(st->fd, TCSAFLUSH, &st->tm) < 0))
		{
		Error("cannot set baud-rate on serial-port: %s", st->portname);
		}

	/* although the use of TCSAFLUSH should discard pending input
	 * data (that may have been received at the wrong baud-rate),
	 * some serial-ports have a race condition that may allow
	 * some such malformed input to get through. Thus we protect
	 * ourselves with a few explicit flushes...
	 */
	for (i = 0; i < 5; ++i)
		tcflush(st->fd, TCIFLUSH);

	/* to cooperate with other drivers when autoprobing, should wait
	 * for printer to signal STAT_IDLE before returning (some
	 * printers choke if you send them something within a upto 1 second
	 * of a baud-rate change)... of course, as we may be talking to
	 * an alien device, we have to time-out the wait (STAT_IDLE
	 * may *never* arrive).
	 */
	if (wait_for_idle(st, 1))
		TRACE((6, "printer idle after baud-rate change"));
	}


static VOID formfeed(STATE *st, BOOL blind)
	{
	TRACE((3, "feed page"));
	send_byte(st, 0xC, blind);
	send_byte(st, 0x1, blind);
	tcdrain(st->fd);
	st->tof = YES;
	if (!blind)
		{
		if (!wait_for_idle(st, 20))
			Error("timeout waiting for printer to become idle");
		}
	}


/* Detect and handle printer fault.
 */
static BYTE check_fault(STATE *st, INT timeout)
	{
	sense(st, timeout);

	while (st->ps & RESET_STAT_MASK)
		{
		BYTE s = st->ps;
 
		/* try to force printer into sane state for the next run...
		 */
		reset_printer(st);

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
		else if (s & STAT_CMD_ERR)
			Error("command-protocol error");
		else /* if (s & STAT_PAPER_JAM) */
			Error("label jammed in printer");
		}

	return (st->ps);
	}


/* Initialise a communications-channel from an open file-descriptor.
 * Sets the (initial) local serial-port attributes and modes.
 */
static VOID init_channel(STATE *st)
	{
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

	/* select "physical" pseudo-output modes
	 */
	st->tm.c_cc[VSTOP] = _POSIX_VDISABLE;
	st->tm.c_cc[VSTART] = _POSIX_VDISABLE;
	st->tm.c_iflag &= ~IXON;

	/* select "logical" input modes (completely raw)...
	 */
	st->tm.c_lflag &= ~(ECHO|ECHOE|ECHOK|ECHONL);
	st->tm.c_lflag &= ~(ISIG|ICANON|IEXTEN);
	st->tm.c_lflag &= ~(TOSTOP);
	st->tm.c_lflag |= NOFLSH;
	st->tm.c_cc[VINTR] = _POSIX_VDISABLE;
	st->tm.c_cc[VQUIT] = _POSIX_VDISABLE;
	st->tm.c_cc[VSUSP] = _POSIX_VDISABLE;

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
	tcflush(st->fd, TCIOFLUSH);
	}


static VOID printlabel(OPD *opd, const ARENA *a)
	{
	STATE *st = opd->private;
	UINT band, slice, r, n, lastband = 0;
	INT pos;

	TRACE((3, "mapping bitmap"));
	for (band = 0; band < st->nbands; band += INTERLEAVE)
		{
		BAND *b = &st->map[band];

		b->min = st->nslices;
		b->max = 0;
		(b+1)->min = st->nslices;
		(b+1)->max = 0;

		for (slice = 0; slice < st->nslices; ++slice)
			{
			b->slices[slice] = 0;
			(b+1)->slices[slice] = 0;
			for (n = 0; n < BAND_HEIGHT; ++n)
				{
				r = (band * BAND_HEIGHT);
				r += (n * INTERLEAVE);
				if (mbm_tstb(a->bm, r, slice))
					{
					b->slices[slice] |= (1 << n);
					b->min = min(b->min, slice);
					b->max = max(b->max, slice);
					lastband = band;
					}
				++r;
				++b;
				if (mbm_tstb(a->bm, r, slice))
					{
					b->slices[slice] |= (1 << n);
					b->min = min(b->min, slice);
					b->max = max(b->max, slice);
					lastband = band + 1;
					}
				--b;
				}
			}
		}
	TRACE((3, "printing bitmap"));

	/* feed one blank band to adjust for mistensioned media;
	 * also forces the printer to check for an "empty" condition!
	 */
	send_byte(st, 0xA, NO);
	st->tof = NO;
	tcdrain(st->fd);
	check_loaded(st);

	send_byte(st, 0xD, NO);	/* force start from left margin... */
	send_byte(st, 0x7, NO);	/* plus... */
	send_byte(st, 2, NO);	/* two slices. */

	/* Note that we print interleaved *pairs* of bands, so adjust
	 * lastband to ensure complete interleave of the final pair...
	 * (based on suggested fix by Werner Panocha).
	 */
	if ((lastband < st->nbands) && (lastband % 2))
		lastband++;

	for (pos = 0, band = 0; band <= lastband; ++band)
		{
		BAND *b = &st->map[band];
		INT shift;	/* must be signed! */

		if (b->max)
			{
			/* move to start of dirty-zone:
			 */
			shift = b->min - pos;
			pos += shift;
			if (shift > 0)
				{
				send_byte(st, 0x7, NO);
				send_byte(st, shift, NO);
				}
			else
				{
				send_byte(st, 0x8, NO);
				send_byte(st, -shift, NO);

				/* deliberately overshoot then reverse back,
				 * to reduce misalignment caused by mechanical
				 * backlash in the printer mechanism...
				 */
				if (pos > 0)
					{
					send_byte(st, 0x8, NO);
					send_byte(st, (pos > 1) ? 2 : 1, NO);
					pos = max((INT)pos - 2, 0);
					}
				}

			/* send band:
			 */
			send_byte(st, 0x3, NO);
			send_byte(st, b->max - pos + 1, NO);
			for (slice = pos; slice <= b->max; ++slice)
				{
				send_byte(st, b->slices[slice], NO);
				++pos;
				}
			}

		/* feed to next band position:
		 */
		st->ps &= ~STAT_IDLE;
		send_byte(st, (band % 2) ? 0xB : 0x9, NO);
		tcdrain(st->fd);
		check_fault(st, 0);
		check_loaded(st);

		/* NOTE: the printer will misfeed if we send feed commands
		 * too quickly, so we wait for a STAT_IDLE indication
		 * between bands...
		 */
		if (!wait_for_idle(st, 8)) /* 8 seconds should be plenty */
			Error("timeout waiting for printer to become idle");
		}

	send_byte(st, 0xD, NO);	/* move to left margin... */
	formfeed(st, NO);	/* and top of form... */
	}



/* connect to the printer, and determine exact model
 * NOTE: a target of "-" means use standard-output for comms.
 */
static PTYPE connect(STATE *st, const CHAR *target)
	{
	PTYPE ptype = PT_NONE;
	const CHAR *portname;
	BOOL locking = YES;

	TRACE((3, "connecting (SLP-Plus family driver)"));
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
	if (!(ptype = ping(st)))
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

	send_byte(st, 0x6, NO); /* full-step mode */
	set_density(st, DENS_DEMILIGHT);
 
	wait_for_idle(st, 2);

	TRACE((3, "connected"));
	return (ptype);
	}


/* disconnect from the display-device
 * (NOTE: this could be called asynchronously)
 */
static VOID disconnect(STATE *st)
	{
	TRACE((3, "disconnecting"));

	if (st)
		{
		if (st->fd >= 0)
			{
			tcflow(st->fd, TCOON); 	/* just in case! */
			tcdrain(st->fd); 	/* just in case! */
			force(st);
			if (!st->tof)
				formfeed(st, YES);

			/* wait upto 4 seconds for printer to become idle...
			 */
			tcflow(st->fd, TCOON);
			send_byte(st, 0x1, YES);
			wait_for_idle(st, 4);

			closetty(st->fd, &st->otm);
			st->fd = -1;
			}
		else
			{
			st->fd = -1;	/* to supress "empty else" warnings */
			TRACE((3, "shortcut disconnect"));
			}
		st->portname = mem_free((VOID *) st->portname);
		}

	TRACE((3, "disconnected"));
	}


static BOOL geometry(const MEDIA *m, GEOMETRY *g)
	{
	USHORT k;

	if (m->width != 28)
		return (NO);

	/* compute geometry in pixels:
	 */
	g->top = PHYS_TOP_MARGIN * VPITCH;
	g->bottom = PHYS_BOTTOM_MARGIN * VPITCH;
	g->right = PHYS_RIGHT_MARGIN * HPITCH;
	g->left = PHYS_LEFT_MARGIN * HPITCH;
 
	g->height = (m->height * VPITCH) - g->top - g->bottom;
	g->width = (m->width * HPITCH) - g->left - g->right;

	/* round to paper-feed units...
	 * ie: ensure integral even number of bands
	 */
	/* SPOOF! should really reduce height, not increase it!!...
	 * but that would either give us a large bottom margin, or would
	 * require the logic to map and print an odd number of bands.
	 * Maybe later - increasing height works properly in almost every case.
	 * In a very few cases, the "too small" bottom margin can cause
	 * the implied trailing formfeed to fall a bit short :(.
	 */
	k = (BAND_HEIGHT * INTERLEAVE) - (g->height % (BAND_HEIGHT * INTERLEAVE));
	TRACE((3, "h=%i+%i w=%i", g->height, k, g->width));
	g->bottom -= k;
	g->height += k;
 
	assert((g->height % BAND_HEIGHT) == 0);
	return (YES);
	}

static DOUBLE hres(OPD *opd)
	{
	return (HPITCH);
	}

static DOUBLE vres(OPD *opd)
	{
	return (VPITCH);
	}


/* open and initialise the printer/device
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
		/* NOTE: we ignore maxbaudval here, these printers are
		 * fixed-speed devices...
		 */
		st = mem_buy(NULL, sizeof(*st), YES);
		st->ps = 0;
		st->fd = -1;
		st->tof = YES;
		st->portname = NULL;
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
	GEOMETRY gg;
	USHORT band;
	STATE *st = opd->private;

	if (!geometry(media, &gg))
		return (NO);
	if (g)
		*g = gg;

	if (a)
		{
		if (a->bm)
			mbm_free(a->bm);
		a->bm = mbm_buy(gg.height, gg.width, YES);
		a->dirty = NO;
		}

	if (opd->target)
		{
		/* allocate linear device-format bitmap:
		 */
		if (st->map)
			{
			for (band = 0; band < st->nbands; ++band)
				mem_free(st->map[band].slices);
			}
		st->nbands = gg.height / BAND_HEIGHT;
		st->nslices = gg.width;
		assert(!(gg.height % BAND_HEIGHT));
		TRACE((4, "nslices=%u, nbands=%u", st->nslices, st->nbands));
		st->map = mem_buy(st->map, st->nbands * sizeof(BAND), YES);
		for (band = 0; band < st->nbands; ++band)
			{
			st->map[band].slices =
				mem_buy(NULL, st->nslices, YES);
			memset(st->map[band].slices, 0, st->nslices);
			st->map[band].min = st->nslices;
			st->map[band].max = 0;
			}
		}
	
	return (YES);
	}


/* deinitialise and close the printer/device
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

/****************************************************************************/

DRV slplus_drv =
	{
	opendev,
	setmedia,
	printlabel,
	closedev,
	hres,
	vres
	};
