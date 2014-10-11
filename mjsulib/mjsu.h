/* MJS Portable C Library.
 * Module: API declarations: data-types, macros, manifest constants etc. 
 * @(#) mjsu.h	3.2	16jun2012 MJS
 *
 * Copyright (c) 1989-1997, 2005 Mike Spooner
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
 *----------------------------------------------------------------------
 *
 * All canonical names (functions, constants, types) have a leading "mjs_"
 * prefix, to prevent collisions with other libraries in the global namespace.
 *
 * Shorthand names (aliases without the prefix) are also defined unless
 * one or more of MJS_CONSTANTS, MJS_TYPES, or MJS_FUNCTIONS are defined.
 */
#ifndef __MJSU_H__
#define __MJSU_H__ 1


#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdarg.h>


/* canonical types and manifest constants:
 */
typedef enum {mjs_NO=0, mjs_FALSE=0, mjs_YES=1, mjs_TRUE=1} mjs_BOOL;
typedef void mjs_VOID;
typedef char mjs_CHAR;
typedef signed char mjs_TINY;
typedef unsigned char mjs_UTINY, mjs_BYTE;
typedef short mjs_SHORT;
typedef unsigned short mjs_USHORT;
typedef int mjs_INT;
typedef unsigned int mjs_UINT;
typedef long mjs_LONG;
typedef unsigned long mjs_ULONG;
typedef float mjs_SINGLE;
typedef double mjs_DOUBLE;

/* structure of a "tagged" pattern-match
 */
typedef struct
	{
	mjs_UINT mlen;
	mjs_CHAR *mtext;
	} mjs_TAGMATCH;

/* structure of an in-core bitmap:
 */
typedef struct
	{
	mjs_USHORT height;	/* in dots */
	mjs_USHORT width;	/* in dots */
	mjs_BYTE *bits;		/* dynamically allocated;
			 	 * number of allocated bytes given by:
				 *      ((height * width) + 7) / 8;
				 * Bit-order is MSB first;
				 * First bit is top-left,
				 * next bit is next-left, etc;
				 * No per-row padding is present;
				 */
	} mjs_MBITMAP;

/* file-format constants for bitmap files:
 */
#define mjs_MBM_MAGIC	"<MBM"
#define mjs_MBM_VERSION	1


/* optional shorthand aliases for manifest constants:
 */
#ifndef MJS_CONSTANTS

#define NO	mjs_NO
#ifndef FALSE
#define FALSE	mjs_FALSE
#endif
#define YES	mjs_YES
#ifndef TRUE
#define TRUE	mjs_TRUE
#endif
#define MBM_MAGIC	mjs_MBM_MAGIC
#define MBM_VERSION	mjs_MBM_VERSION

#endif


/* optional shorthand aliases for canonical types:
 */
#ifndef MJS_TYPES

typedef mjs_BOOL	BOOL;
typedef mjs_VOID	VOID;
typedef mjs_CHAR	CHAR;
typedef mjs_TINY	TINY;
typedef mjs_UTINY	UTINY;
typedef mjs_BYTE	BYTE;
typedef mjs_SHORT	SHORT;
typedef mjs_USHORT	USHORT;
typedef mjs_INT		INT;
typedef mjs_UINT	UINT;
typedef mjs_LONG	LONG;
typedef mjs_ULONG	ULONG;
typedef mjs_SINGLE	SINGLE;
typedef mjs_DOUBLE	DOUBLE;
typedef mjs_TAGMATCH	TAGMATCH;
typedef mjs_MBITMAP	MBITMAP;

#endif


/* map full API-level external names into minimal-length link-level names
 * (ANSI C89 allows external symbols to be limited to 6 initial *significant*
 * characters, due to system/tools (eg: assembler) limitations, so we must
 * arrange for all externally-visible symbols to be distinct in the first
 * six characters
 */
#define mjs_usage		mjusge
#define mjs_remark		mj_rmk
#define mjs_vremark		mjvrmk
#define mjs_warning		mj_wrn
#define mjs_vwarning		mjvwrn
#define mjs_error		mj_err
#define mjs_verror		mjverr
#define mjs_panic		mjpani
/* #define mjs_verrmsg		mjverm */
#define mjs_whatami		mjwami
#define mjs_getflags		mjgtfl
#define mjs_mem_buy		mjmemb
#define mjs_mem_free		mjmemf
#define mjs_str_dup		mjstrd
#define mjs_vec_buy		mjvecb
#define mjs_vec_dup		mjvecd
#define mjs_vec_free		mjvecf
#define mjs_amatch		mjamat
#define mjs_match		mj_mat
#define mjs_pattern		mjpatt
#define mjs_cpystr		mjcpst
#define mjs_mbm_buy		mjbmby
#define mjs_mbm_free		mjbmfr
#define mjs_mbm_clr		mjbmca
#define mjs_mbm_set		mjbmsa
#define mjs_mbm_rev		mjbmra
#define mjs_mbm_height		mjbmht
#define mjs_mbm_width		mjbmwd
#define mjs_mbm_tstb		mjbmtb
#define mjs_mbm_setb		mjbmsb
#define mjs_mbm_clrb		mjbmcb
#define mjs_mbm_revb		mjbmrb
#define mjs_mbm_load		mjbmld
#define mjs_mbm_save		mjbmsv
#define mjs_lstos		mjls2s
#define mjs_stols		mjs2ls
#define mjs_lstol		mjls2l
#define mjs_ltols		mjl2ls
#define mjs_rtw_short_hash	mjshth
#define mjs_rtw_long_hash	mjlngh
#define mjs_strhash		mjstrh
#define mjs_strtos		mjst2s

 
/* API functions:
 */
mjs_VOID mjs_usage(mjs_CHAR *, ...);
mjs_UINT mjs_remark(mjs_CHAR *, ...);
mjs_UINT mjs_vremark(mjs_CHAR *, va_list);
mjs_UINT mjs_warning(mjs_CHAR *, ...);
mjs_UINT mjs_vwarning(mjs_CHAR *, va_list);
mjs_VOID mjs_error(mjs_CHAR *, ...);
mjs_VOID mjs_verror(mjs_CHAR *, va_list);
mjs_VOID mjs_panic(mjs_CHAR *, ...);
mjs_VOID mjs_verrmsg(mjs_CHAR *, mjs_CHAR *, va_list);
const mjs_CHAR *mjs_whatami(mjs_VOID);

mjs_CHAR *mjs_getflags(mjs_INT *, mjs_CHAR ***, mjs_CHAR *, mjs_CHAR *, ...);

mjs_VOID *mjs_mem_buy(mjs_VOID *, mjs_UINT, mjs_BOOL);
mjs_VOID *mjs_mem_free(mjs_VOID *);

mjs_CHAR *mjs_str_dup(const mjs_CHAR *, mjs_BOOL);

mjs_CHAR **mjs_vec_buy(mjs_CHAR **, mjs_CHAR *, mjs_BOOL);
mjs_CHAR **mjs_vec_dup(mjs_CHAR **, mjs_BOOL);
mjs_CHAR **mjs_vec_free(mjs_CHAR **);

mjs_BOOL mjs_amatch(mjs_CHAR *buf, mjs_UINT n, mjs_UINT idx, mjs_UINT *pmend,
				mjs_USHORT *pat, mjs_TAGMATCH *pmsub);
mjs_BOOL mjs_match(mjs_CHAR *buf, mjs_UINT n, mjs_USHORT *pat);
mjs_CHAR *mjs_pattern(mjs_USHORT *pat, mjs_CHAR delim, mjs_CHAR *expr);

mjs_CHAR *mjs_cpystr(mjs_CHAR *, ...);

mjs_MBITMAP *mjs_mbm_buy(mjs_USHORT h, mjs_USHORT w, mjs_BOOL);
mjs_MBITMAP *mjs_mbm_free(mjs_MBITMAP *);
mjs_MBITMAP *mjs_mbm_clr(mjs_MBITMAP *);
mjs_MBITMAP *mjs_mbm_set(mjs_MBITMAP *);
mjs_MBITMAP *mjs_mbm_rev(mjs_MBITMAP *);
mjs_USHORT mjs_mbm_height(const mjs_MBITMAP *);
mjs_USHORT mjs_mbm_width(const mjs_MBITMAP *);
mjs_BOOL mjs_mbm_tstb(const mjs_MBITMAP *, mjs_USHORT r, mjs_USHORT c);
mjs_VOID mjs_mbm_setb(mjs_MBITMAP *, mjs_USHORT r, mjs_USHORT c);
mjs_VOID mjs_mbm_clrb(mjs_MBITMAP *,  mjs_USHORT r,  mjs_USHORT c);
mjs_BOOL mjs_mbm_revb(mjs_MBITMAP *, mjs_USHORT r, mjs_USHORT c);
mjs_MBITMAP *mjs_mbm_load(FILE *);
mjs_BOOL mjs_mbm_save(mjs_MBITMAP *, FILE *);

mjs_USHORT mjs_lstos(mjs_USHORT);
mjs_USHORT mjs_stols(mjs_USHORT);
mjs_ULONG mjs_lstol(mjs_ULONG);
mjs_ULONG mjs_ltols(mjs_ULONG);

mjs_USHORT mjs_rtw_short_hash(const mjs_BYTE *key, size_t len);
mjs_ULONG mjs_rtw_long_hash(const mjs_BYTE *key, size_t len);
mjs_USHORT mjs_strhash(const mjs_CHAR *str);

short mjs_strtos(const char *str, char **endptr, int base);


/* optional shorthand aliases for functions and macros:
 */
#ifndef MJS_FUNCTIONS

#define usage		mjs_usage
#define remark		mjs_remark
#define vremark		mjs_vremark
#define warning		mjs_warning
#define vwarning	mjs_vwarning
#define error		mjs_error
#define verror		mjs_verror
#define panic		mjs_panic
#define verrmsg		mjs_verrmsg
#define whatami		mjs_whatami
#define getflags	mjs_getflags
#define mem_buy		mjs_mem_buy
#define mem_free	mjs_mem_free
#define str_dup		mjs_str_dup
#define cpystr		mjs_cpystr
#define vec_buy		mjs_vec_buy
#define vec_dup		mjs_vec_dup
#define vec_free	mjs_vec_free
#define amatch		mjs_amatch
#define match		mjs_match
#define pattern		mjs_pattern
#define mbm_buy		mjs_mbm_buy
#define mbm_free	mjs_mbm_free
#define mbm_clr		mjs_mbm_clr
#define mbm_set		mjs_mbm_set
#define mbm_rev		mjs_mbm_rev
#define mbm_width	mjs_mbm_width
#define mbm_height	mjs_mbm_height
#define mbm_tstb	mjs_mbm_tstb
#define mbm_setb	mjs_mbm_setb
#define mbm_clrb	mjs_mbm_clrb
#define mbm_revb	mjs_mbm_revb
#define mbm_load	mjs_mbm_load
#define mbm_save	mjs_mbm_save
#define lstol		mjs_lstol
#define lstos		mjs_lstos
#define ltols		mjs_ltols
#define stols		mjs_stols
#define rtw_long_hash	mjs_rtw_long_hash
#define rtw_short_hash	mjs_rtw_short_hash
#define strhash		mjs_strhash
#define strtos		mjs_strtos
#endif
 

#ifdef __cplusplus
}
#endif

#endif
