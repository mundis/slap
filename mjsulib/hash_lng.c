/* MJS Portable C Library.
 * Module: long-hash function.
 * @(#) hash_lng.c	1.0	10mar2005 MJS
 *
 * Copyright (c) 2005 Mike Spooner
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

/* functions to generate highly-distributed and highly-random unsigned-long
 * hash values from user-supplied arrays of bytes ("keys").
 *
 * The same input key *always* generates the same output hash value,
 * within a single process. Generated hash-values are *not* guaranteed
 * portable *between* processes (so don't bother writing them to a file!),
 * or even between successive runs of the same program (thats a new process,
 * geddit?).
 *
 * Uses a random-table walk algorithm: extremely fast, requires just 256 bytes
 * of static memory (read-only), and produces randomly-distributed, and
 * highly-distributed outputs.
 *
 * Note that the execution-time is linearly proportional to the length of
 * the input key, so using *very* long keys will slow the algorithm down.
 *
 * These functions have been written very carefully to be portable to machines
 * with eg: 9-bit bytes, 33-bit words, 11/16/17/32/49/64/121/769/3343- and
 * 893797-bit longs (and so on), longs of any size down to byte (whether that
 * be 8-bit, 9-bit or anything else), and so on and so on.
 *
 * As originally written, this code is good for 8-bit thru 1048576-bit longs.
 * So don't mess with it unless you understand how that portability has been
 * achieved.
 *
 * The only hard assumptions are:
 *
 *	that unsigned char has *at least* 8 bits
 * 	that long is no smaller than short
 * 	that short is no smaller than unsigned char
 *
 * IMHO, both of the latter are guaranteed by the ANSI C Language standard,
 * ANSI X3.159-1989 and also by the pre-ANSI de-facto standard,
 * "The C Programming Language" by Kernighan and Ritchie. The former is
 * almost universally satisfied (there was a C compiler in the late 1970's
 * for a bitslice processor that used 6-bit unsigned char, but that is the
 * ONLY exception I have been able to discover). Note that even with
 * truly tiny unsigned chars (<8 bits), the algorithm still does the
 * right thing (produces a valid, reliable hash value), but sacrifices
 * some degree of spectral distribution in the output.
 *
 * NOTE: *all* bits of the output are significant, eg: if you have 8-bit chars
 * and 37-bit longs, *all* 37 bits are written, not just the bottom 32 bits.
 * This is very important, as hash functions are
 * sometimes used to generate random integers (by passing in random long
 * strings of entropy) of which only a certain number of bits are desired.
 * For instance, to generate a random number in the range 0-511, people
 * have been known to do something like:
 *
 *	char *s = get_entropy();
 *	short idx = rtw_long_hash(s, strlen(s)) & 0x1FF;
 *						 ^^^^^^^
 * which assumes that *all* bits returned by rtw_long_hash() are equally
 * significant. They are. Keep it so!
 *
 * TODO: it may be possible to "break out" the nested-loop structure by
 * using Duff's device, which could make the algorithm even faster.
 */
#define UNROLL
#undef DEBUG


#include <string.h>
#include <limits.h>
#include "mjsu.h"
#include "mjsuimpl.h"

#ifdef DEBUG
#include <assert.h>
#endif

/* compute an unsigned long hash value of specified (opaque) input key:
 */
ULONG rtw_long_hash(const BYTE *key, size_t len)
	{
	/* manifest constant number-of-octets-per-unsigned-long, *rounded up*
	 * so that any leftover partial-octet is counted.
	 * The C compiler can evaluate at this compile-time rather than run-time
	 * and thus is allowed to "optimise-out" the switch cases that can't
	 * apply, in the unrolled-loop code, below.
	 */
	#define STRIDE ((sizeof(ULONG) / sizeof(BYTE)) + \
			((ULONG_MAX % OCTET_MAX) ? 1 : 0))
	
	ULONG output, octets[STRIDE];
	size_t n, i, j;

	#ifdef DEBUG
	assert(STRIDE > 0);
	#endif

	if (!len)
		return ~0L;

	/* to preserve spectral distribution even in the face of *very* short
	 * keys (len < stride), we *must* use part of the key's content as a
	 * base index into the hashmap, which we then table-walk with a
	 * variable offset to get a randomised starting value for each element
	 * of the octets[] array..
	 */
	i = min(len, STRIDE);
	--i;
	j = 0;
	switch (STRIDE)
		{
	#ifdef UNROLL
	case 8:
		octets[7] = hashmap[(((size_t) key[i]) + ++j) & 0xFF];
		octets[6] = hashmap[(((size_t) key[i]) + ++j) & 0xFF];
		octets[5] = hashmap[(((size_t) key[i]) + ++j) & 0xFF];
		octets[4] = hashmap[(((size_t) key[i]) + ++j) & 0xFF];
		/* fall thru */
	case 4:
		octets[3] = hashmap[(((size_t) key[i]) + ++j) & 0xFF];
		octets[2] = hashmap[(((size_t) key[i]) + ++j) & 0xFF];
		/* fall thru */
	case 2:
		octets[1] = hashmap[(((size_t) key[i]) + ++j) & 0xFF];
		octets[0] = hashmap[(((size_t) key[i]) + ++j) & 0xFF];
		break;
	#endif
	default:
		for (n = 0; n < STRIDE; ++n)
			octets[n] = hashmap[(((size_t) key[i]) + ++j) & 0xFF];
		break;
		}

	/* for each byte of the key, tablewalk each byte of the pending
	 * output...
	 */
	for (i = 0; i < len; ++i)
		{
		switch (STRIDE)
			{
		#ifdef UNROLL
		case 8:
			octets[7] = hashmap[(octets[7] ^ key[i]) & 0xFF] & 0xFF;
			octets[6] = hashmap[(octets[6] ^ key[i]) & 0xFF] & 0xFF;
			octets[5] = hashmap[(octets[5] ^ key[i]) & 0xFF] & 0xFF;
			octets[4] = hashmap[(octets[4] ^ key[i]) & 0xFF] & 0xFF;
			/* fall thru */
		case 4:
			octets[3] = hashmap[(octets[3] ^ key[i]) & 0xFF] & 0xFF;
			octets[2] = hashmap[(octets[2] ^ key[i]) & 0xFF] & 0xFF;
			/* fall thru */
		case 2:
			octets[1] = hashmap[(octets[1] ^ key[i]) & 0xFF] & 0xFF;
			octets[0] = hashmap[(octets[0] ^ key[i]) & 0xFF] & 0xFF;
			break;
		#endif
		default:
			for (n = 0; n < STRIDE; ++n)
				octets[n] = hashmap[(octets[n] ^ key[i]) & 0xFF] & 0xFF;
			break;
			}
		}

	output = 0;
	/* unrolling the output-accumulation loop doesn't seem to
	 * improve performance to any measurable extent...
	 */
	for (n = 0; n < STRIDE; ++n)
		{
		output <<= 8;
		output |= octets[n];
		#ifdef DEBUG
		remark("rtw_long_hash output pass %i => 0x%0*lx",
			STRIDE - n, STRIDE << 1, output);
		#endif
		}
	return (output);
	#undef STRIDE
	}
