/* MJS Portable C Library.
 * Module: private internal random-table-walk hash map
 * @(#) hashmap.c	1.0	10mar2005 MJS
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
#include "mjsu.h"
#include "mjsuimpl.h"

/* a random mapping of 8-bit values:
 */
const BYTE hashmap[256] =
	{
	 57,141,117, 50,138, 42,178,102, 27,167,192, 69,187,179,143,101,
	174, 44, 30,129,105,108,223, 18, 64,116,145,229,232,214,  9,132,
	150,100, 95, 37, 84,122,112,202, 59,165, 21, 49, 38, 46,238,  6,
	 39,113, 40,135, 32,189, 66,177,221,114,247,180,144, 63,160,125,
	131,219,244, 47, 10, 55, 87,111, 94,210,121,156, 29, 34,212,226,
	249,159,155, 76,235,231, 98,197, 43,236, 77,  1, 54,151,204,147,
	211,130,216,251,  5,220, 52, 28,  8,222,  7, 89, 81, 71,190,120,
	104,209, 96,255,  0, 67,188,215,248,161,118, 23,119,217, 88,207,
	140,234, 92, 36,162,139,171, 17, 56,  4,157, 80,191,250,241,126,
	239,163,148,205, 74,153, 60,166, 93, 83, 41, 61, 86,109,173,146,
	199, 72,227,106,103, 48,242, 99, 12,170,134, 73,233,184, 91, 16,
	  2, 65,245,136,194,154,  3,196,243,158,169, 13,198,127,224,183,
	 90,254,200, 97, 14,181,237,201,228,185,110,193,164,253,175, 82,
	149, 53,252,213,182, 79, 68, 11, 20, 33,176,123, 51,152, 85, 25,
	218,186, 75, 62,137, 35,203,133,240,115, 26, 78,124,246, 31, 58,
	 19,107, 24,225,142,230,128, 15,208, 22,168,172,195, 45,206, 70
	};

