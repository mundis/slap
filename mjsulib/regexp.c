/* MJS Portable C Library.
 * Module: regular-expression routines. Encoding, Matching, et al.
 * @(#) regexp.c  2.5 03sep92 MJS
 *
 * Copyright (c) 1991, 1992 Mike Spooner
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
 * Source-code conforms to ANSI standard X3.159-1989.
 */
#include <stdlib.h>
#include <ctype.h>
#include "mjsu.h"


#define isodigit(c)     (isdigit((int)(c)) && ((c) != '8') && ((c) != '9'))

/* Regular-expression encodings:
 */
#define CCHAR	(1<<8)	/* literal character */
#define ANY	(2<<8)	/* any character */
#define SBOL	(3<<8)	/* beginning of line */
#define SEOL	(4<<8)	/* end of line */
#define CLOSE	(5<<8)	/* reflexive closure */
#define CCL	(6<<8)	/* begin character class */
#define NCCL	(7<<8)	/* begin negated character class */
#define RANGE	(8<<8)	/* range within character class */
#define CCLEND	(9<<8)	/* end of character class */
#define PEND	(10<<8)	/* pattern end */
#define RPAR	(19<<8)	/* right parenthesis */
#define LPAR	(20<<8)	/* left parenthesis */


/* widen a character code into a USHORT, avoiding sign-extension...
 */
static USHORT widen(CHAR ccode)
	{
	UTINY uccode = ccode;

	return ((USHORT) uccode);
	}


/* return length of subpat in offset
 */
static BOOL mtchccl(CHAR c, USHORT *pat, UINT *offset)
	{
	USHORT rlc, ruc;
	USHORT *stpat = pat;
	BOOL matched;

	for (matched = NO; *pat != CCLEND; )
		{
		switch (*pat)
			{
		case CCHAR:
			if (widen(c) == *++pat)
				matched = YES;
			++pat;
			break;
		case RANGE:
			for (rlc = *++pat, ruc = *++pat; rlc <= ruc; ++rlc)
				{
				if (widen(c) == rlc)
					matched = YES;
				}
			++pat;
			break;
			}
		}
	*offset = pat - stpat + 1;
	return (matched);
	}

/* try to match one sub pattern
 */
static BOOL omatch(CHAR *buf, UINT n, UINT *pindx, 
	USHORT **pat, TAGMATCH *pmsub)
	{
	CHAR *pp;
	USHORT *ppat;
	INT bump;
	UINT offset;

	offset = 0;
	pp = &buf[*pindx];
	ppat = *pat;
	bump = -1;
	switch (*ppat)
		{
	case SBOL:
		if (*pindx == 0)
			{
			bump = 0;
			++ppat;
			}
		break;
	case ANY:
		if ((*pindx < n) && (*pp != '\n'))
			bump = 1;
		++ppat;
		break;
	case SEOL:
		if ((*pp == '\n') || (*pindx == n))
			{
			bump = 0;
			++ppat;
			}
		break;
	case CCL:
		if ((*pindx < n) && mtchccl(*pp, ++ppat, &offset))
			bump = 1;
		ppat += offset;
		break;
	case NCCL:
		if ((*pindx < n) && (*pp != '\n') &&
			!mtchccl(*pp, ++ppat, &offset))
			{
			bump = 1;
			}
		ppat += offset;
		break;
	case LPAR:
		if (pmsub)
			{
			pmsub[ppat[1]].mtext = pp;
			pmsub[ppat[1]].mlen = 0;
			}
		ppat += 2;
		bump = 0;
		break;
	case RPAR:
		if (pmsub)
			pmsub[ppat[1]].mlen = pp - pmsub[ppat[1]].mtext;
		ppat += 2;
		bump = 0;
		break;
	case CCHAR:
		*pat += 2;
		if ((*pindx < n) && (widen(*pp) == ppat[1]))
			{
			++*pindx;
			return (YES);
			}
		return (NO);
		}
	*pat = ppat;
	if (0 <= bump)
		{
		*pindx += bump;
		return (YES);
		}
	else
		return (NO);
	}


/* look for match anchored at index
 */
BOOL amatch(CHAR *buf, UINT n, UINT indx, UINT *pmend,
	USHORT *pat, TAGMATCH *pmsub)
	{
	UINT i;
	USHORT *rpat;

	while (*pat != PEND)
		{
		if (*pat == CLOSE)
			{
			for (rpat = pat + 1, i = indx; i < n; rpat = pat + 1)
				{
				if (!omatch(buf, n, &i, &rpat, NULL))
					break;
				}
			/* NOTE: must be careful to test for (i == indx) 
			 * at base of loop, to avoid unsigned wraparound 
			 * problems...
			 */
			for ( ; ; --i)
				{
				if (amatch(buf, n, i, pmend, rpat, pmsub))
					return (YES);
				if (i == indx)
					{
					/* could not match remainder of
					 * pattern, even with smallest
					 * possible closure match...
					 */
					return (NO);
					}
				}
			}
		else if (!omatch(buf, n, &indx, &pat, pmsub))
			return (NO);
		}
	*pmend = indx;
	return (YES);
	}


/* look for unanchored match
 */
BOOL match(CHAR *buf, UINT n, USHORT *pat)
	{
	UINT indx;
	UINT junk;

	if (*pat == SBOL)
		return (amatch(buf, n, 0, &junk, pat, NULL));
	for (indx = 0; indx < n; ++indx)
		{
		if ((widen(buf[indx]) != pat[3]) && (pat[2] == CCHAR))
			;
		else if (amatch(buf, n, indx, &junk, pat, NULL))
			return (YES);
		}
	return (NO);
	}

/* encode escape sequence
 */
static UTINY doesc(CHAR **pp)
	{
	CHAR *p = *pp;
	UTINY code;
	
	if (!p[1])
		code = *p;
	else if (isodigit(p[1]))
		{
		p++;
		code = *p - '0';                
		if (isodigit(p[1]))
			{
			p++;
			code <<= 3;
			code += *p - '0';
			if (isodigit(p[1]))
				{
				p++;
				code <<= 3;
				code += *p - '0';
				}
			}
		}
	else 
		{
		p++;
		switch ((UINT) *p)
			{
		case 'b': 
			code = '\b'; 
			break;
		case 'f': 
			code = '\f'; 
			break;
		case 'n': 
			code = '\n'; 
			break;
		case 'r': 
			code = '\r'; 
			break;
		case 't': 
			code = '\t'; 
			break;
		case 'v': 
			code = '\v'; 
			break;
		default:
			code = *p;
			break;
			}
		}

	*pp = p;
	return (code);
	}


/* form character class sub pattern
 */
static USHORT *getccl(CHAR **pp, USHORT *pat)
	{
	BOOL ranok;
	CHAR *p;

	p = *pp;
	if (*++p == '!')
		{
		*pat++ = NCCL;
		++p;
		}
	else
		*pat++ = CCL;
	for (ranok = NO; *p != ']' && *p; ++p)
		{
		switch ((UINT)*p)
			{
		case '\\':
			*pat++ = CCHAR;
			*pat++ = widen(doesc(&p));
			ranok = YES;
			break;
		case '-':
			if (ranok && p[1] != ']')
				{
				CHAR code;
				code = (*++p == '\\') ? doesc(&p) : *p;
				pat[-2] = RANGE;
				*pat++ = widen(code);
				ranok = NO;
				break;
				}
		default:
			*pat++ = CCHAR;
			*pat++ = widen(*p);
			ranok = YES;
			}
		}
	*pat++ = CCLEND;
	*pp = (!p) ? --p: p;
	return (pat);
	}

CHAR *pattern(USHORT *pat, CHAR delim, CHAR *p)
	{
	INT nlp, parstack[10], *pstack;
	USHORT *last, *t;

	nlp = 0;
	pstack = parstack;
	if (*p == '^')
		{
		++p;
		*pat++ = SBOL;
		}
	*pat++ = LPAR;
	*pat++ = 0;
	for (last = NULL; *p && *p != delim; ++p)
		{
		if (*p != '^')
			last = pat;
		switch (*p)
			{
		case '?':
			*pat++ = ANY;
			break;
		case '*':
			*pat++ = CLOSE;
			*pat++ = ANY;
			break;
		case '$':
			if (p[1] == delim || !p[1])
				*pat++ = SEOL;
			else
				{
				*pat++ = CCHAR;
				*pat++ = widen('$');
				}
			break;
		case '^':
			if (!last || *last == LPAR || *last == RPAR ||
				*last == CLOSE)
				{
				*pat++ = CCHAR;
				*pat++ = widen(*p);
				}
			else
				{
				for (t = ++pat; t != last; --t)
					t[0] = t[-1];
				*last = CLOSE;
				}
			break;
		case '[':
			pat = getccl(&p, pat);
			break;
		case '\\':
			if (p[1] == '(')
				{
				if (0 <= ++nlp && nlp < 10)
					{
					*pstack++ = nlp;
					*pat++ = LPAR;
					*pat++ = nlp;
					}
				}
			else if (p[1] == ')')
				{
				if (10 <= nlp)
					--nlp;
				else if (0 <= nlp)
					{
					--pstack;
					*pat++ = RPAR;
					*pat++ = *pstack;
					}
				}
			else
				{
				*pat++ = CCHAR;
				*pat++ = widen(doesc(&p));
				}
			break;
		default:
			*pat++ = CCHAR;
			*pat++ = widen(*p);
			}
		}
	*pat++ = RPAR;
	*pat++ = 0;
	*pat++ = PEND;
	*pat = 0;
	return ((*p == delim && parstack == pstack) ? p : NULL);
	}
