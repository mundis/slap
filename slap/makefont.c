/* MJS BitFont library, version 1.
 * Module: import a font from an X11 "showfont" dump
 *
 * Copyright (c) 1997 by Mike Spooner
 *------------------------------------------------------------------------
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
 *-----------------------------------------------------------------------
 */
static const char * const module = "@(#)" __FILE__ " 3.7 30jun1997 MJS";


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "mjsu.h"
#include "mjsfont.h"

#define max(a, b)	(((a) > (b)) ? (a) : (b))
#define min(a, b)	(((a) < (b)) ? (a) : (b))
static BOOL verbose = NO;


static CHAR *trim(CHAR *s)
	{
	CHAR *p;

	while (isspace(*s))
		++s;
	p = s + strlen(s) - 1;
	while (isspace(*p))
		*p-- = '\0';
	return (s);
	}

static BOOL import_glyph(FILE *pf, FONT *f)
	{
	CHAR buf[BUFSIZ];
	GLYPH *g;
	ULONG off;
	INT idx, i, j;
	INT n;

	if (!fgets(buf, BUFSIZ, pf))
		return (NO);
	if (sscanf(buf, "char #%u", &idx) != 1)
		return (NO);
	if (idx > 255)
		error("glyph index (%u) out of range", idx);
	
	g = &f->glyphs[idx];
	n = fscanf(pf, "Left: %hd Right: %hd Ascent: %hd Descent: %hd Width: %hd\n",
		&g->lbearing, &g->rbearing, &g->ascent, &g->descent, &g->width);
	if (n != 5)
		return (NO);

	if ((g->ascent + g->descent) < 0)
		error("impossible vertical metrics");
	if (g->rbearing < g->lbearing)
		error("impossible horizontal metrics");
		
	if (verbose)
		{
		printf("idx %u left %d right %d ascent %d descent %d width %d ",
				idx, g->lbearing, g->rbearing,
				g->ascent, g->descent, g->width);
		printf("bitmap %ux%u\n",
			(g->rbearing - g->lbearing), (g->ascent + g->descent));
		}
	g->bm = mbm_buy(g->ascent + g->descent, g->rbearing - g->lbearing, YES);

	if (((g->ascent + g->descent) > 0) &&
		(g->rbearing > g->lbearing))
		{
		for (i = 0; i < (g->ascent + g->descent); ++i)
			{
			if (!fgets(buf, BUFSIZ, pf))
				error("unexpected end-of-file");
			for (j = 0; j < (g->rbearing - g->lbearing); ++j)
				{
				switch (buf[j])
					{
				case '#':
					mbm_setb(g->bm, i, j);
				case '-':
					break;
				default:
					error("source bitmap does not " \
						"match metrics");
					}
				}
			}
		}

	off = ftell(pf);
	if (fgets(buf, BUFSIZ, pf))
		{
		if (strcmp(buf, "Nonexistent character\n"))
			fseek(pf, off, SEEK_SET);
		}
	return (YES);
	}

static FONT *import(FILE *pf)
	{
	CHAR buf[8192], weight[FN_MAX], slant[2], cright[8192];
	LONG off;
	UINT n;
	FONT *f = NULL;

	f = mem_buy(f, sizeof(*f), YES);
	memset(f, 0, sizeof(*f));

	fscanf(pf, "opened font -%[^\n]\n", buf);
	if (verbose)
		printf("XLFD: -%s\n", buf);

	/* get defaults for optional font-prop input-records: */
	sscanf(buf, "-%n[^-]-%[^-]-%[^-]-%[^-]-", 
		f->name, weight, slant);

	/* and parse remainder: */
	fgets(buf, BUFSIZ, pf);	/* Direction */
	fgets(buf, BUFSIZ, pf);	/* Range */
	fscanf(pf, "Default char: %u\n", &n);
	f->dg = n;

	fscanf(pf, "Min bounds:\nLeft: %*d Right: %*d Ascent: %*d Descent: %*d Width: %*d\n");
	fscanf(pf, "Max bounds:\nLeft: %*d Right: %*d Ascent: %hd Descent: %hd Width: %*d\n",
		&f->ascent, &f->descent);
	fscanf(pf, "Font Ascent: %*d Font Descent: %*d\n");

	f->name[0] = weight[0] = slant[0] = '\0';
	strcpy(cright, "none");
	off = ftell(pf);
	while (fgets(buf, BUFSIZ, pf) && isupper(buf[0]))
		{
		sscanf(buf, "FAMILY_NAME %[^\n]\n", f->name);
		sscanf(buf, "WEIGHT_NAME %[^\n]\n", weight);
		sscanf(buf, "SLANT %2s\n", slant);
		if (!strncmp(buf, "COPYRIGHT", strlen("COPYRIGHT")))
			strcpy(cright, &buf[strlen("COPYRIGHT")]);
		off = ftell(pf);
		}
	fseek(pf, off, SEEK_SET);

	if (!strlen(f->name))
		return (NULL);
	if (strlen(weight))
		{
		strcat(f->name, "-");
		strcat(f->name, weight);
		}
	if (slant[0] == 'i')
		{
		strcat(f->name, "-");
		strcat(f->name, "italic");
		}
	else if (slant[0] == 'o')
		{
		strcat(f->name, "-");
		strcat(f->name, "oblique");
		}

	if (verbose)
		{
		printf("FONT: %s\n", f->name);
		printf("(%s)\n", trim(cright));
		}

	while (import_glyph(pf, f))
		;
	for (n = 0; n < NGLYPHS; ++n)
		{
		GLYPH *g = &f->glyphs[n];
		if (!g->bm)
			{
			if (verbose)
				{
				remark("undef char #%u: " \
						"left %d right %d " \
						"ascent %d descent %d " \
						"width %d",
					n,
					g->lbearing, g->rbearing,
					g->ascent, g->descent,
					g->width);
				}
			g->bm = mbm_buy(0, 0, YES);
			if (verbose)
				{
				remark("undef char #%u: " \
						"left %d right %d " \
						"ascent %d descent %d " \
						"width %d",
					n,
					g->lbearing, g->rbearing,
					g->ascent, g->descent,
					g->width);
				}
			}
		}
	return (f);
	}

INT main(UINT ac, CHAR **av)
	{
	FILE *pf;
	FONT *f;

	setbuf(stdout, NULL);

	getflags(&ac, &av, "v", "showfont_dump_file   MBF_file", &verbose);
	if (!*av)
		error("showfont_dump_file name expected");
	if (!(pf = fopen(*av, "r")))
		error("cannot read file: %s", *av);
	f = import(pf);
	fclose(pf);
	if (!f)
		error("font import failed from file: %s", *av);

	++av;
	if (!*av)
		error("MBF_file name expected");
	if (!(pf = fopen(*av, "wb")))
		error("cannot create file: %s", "foo");
	if (!savefont(f, pf))
		{
		fclose(pf);
		remove(*av);
		error("cannot save font");
		}
	fclose(pf);

	dropfont(f);

	if (verbose)
		{
		if (!(pf = fopen(*av, "rb")))
			warning("cannot read file: %s", *av);
		setbuf(pf, NULL);
		if (!(f = loadfont(pf)))
			error("cannot load font: %s", *av);
		fclose(pf);
		dumpchar(f, '@');
		dumpchar(f, ']');
		dropfont(f);
		}

	return (EXIT_SUCCESS);
	}	
