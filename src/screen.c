/*
    $Id$

    psid64 - create a C64 executable from a PSID file
    Copyright (C) 2001  Roland Hermans <rolandh@users.sourceforge.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/****************************************************************************
 *                             I N C L U D E S                              *
 ****************************************************************************/

#include <ctype.h>
#include <stdarg.h>
#include <string.h>

#include "glib.h"

#include "screen.h"


/****************************************************************************
 *                           G L O B A L   D A T A                          *
 ****************************************************************************/


/****************************************************************************
 *                     L O C A L   D E F I N I T I O N S                    *
 ****************************************************************************/

#define OFFSET(x,y)			(x + (40 * y))

#define WRAP_CHARS			"-,.)}>"


/****************************************************************************
 *                            L O C A L   D A T A                           *
 ****************************************************************************/

static BYTE             iso2scr (BYTE c);


/****************************************************************************
 *                       L O C A L   F U N C T I O N S                      *
 ****************************************************************************/

static                  BYTE
iso2scr (BYTE c)
{
    switch (c)
    {
    case '@':
	return 0x00;
    case '_':
	return 0x64;
    case '^':
	return 0x1e;
    case '[':
	return 0x1b;
    case ']':
	return 0x1d;
    case '\\':
    case '{':
    case '}':
    case '|':
	return '?';
    }

    if (c < 0x20)
    {
	return c + 0x80;
    }
    else if (c < 0x60)
    {
	return c;
    }
    else if (c < 0x80)
    {
	return c - 0x60;
    }

    return '?';
}


/****************************************************************************
 *                      G L O B A L   F U N C T I O N S                     *
 ****************************************************************************/

void
screen_clear (BYTE * p_screen)
{
    int                     i;
    BYTE                    c;

    c = iso2scr (' ');
    for (i = 0; i < SCREEN_SIZE; i++)
    {
	p_screen[i] = c;
    }
}


void
screen_printf (BYTE * p_screen, int x, int y, char *p_format, ...)
{
    va_list                 va;
    char                   *p_str;
    char                   *p;
    int                     offs;

    va_start (va, p_format);
    p_str = g_strdup_vprintf (p_format, va);
    va_end (va);

    p = p_str;
    offs = OFFSET (x, y);
    while (*p && (offs < SCREEN_SIZE))
    {
	p_screen[offs++] = iso2scr (*p++);
    }

    g_free (p_str);
}


void
screen_wrap (BYTE * p_screen, int x, int y, char *p_str)
{
    int                     i;
    int                     n;

    while ((*p_str) && (x < SCREEN_COLS) && (y < (SCREEN_ROWS - 2)))
    {
	/* skip leading spaces */
	while (*p_str && isspace (*p_str))
	{
	    p_str++;
	}

	/* determine length of next 'word' */
	n = 0;
	while (p_str[n] && !isspace (p_str[n])
	       && !strchr (WRAP_CHARS, p_str[n]))
	{
	    n++;
	}
	if (strchr (WRAP_CHARS, p_str[n]))
	{
	    n++;
	}

	/* put 'word' on next line if it does not fit and its length is less
	   than the screen width */
	if (((x + n) > SCREEN_COLS) && (n < SCREEN_COLS))
	{
	    x = 0;
	    y++;
	}

	if (y < (SCREEN_ROWS - 2))
	{
	    if ((x + n) > SCREEN_COLS)
	    {
		n = SCREEN_COLS - x;
	    }

	    for (i = 0; i < n; i++)
	    {
		p_screen[OFFSET (x, y) + i] = iso2scr (p_str[i]);
	    }
	    p_str = p_str + n;
	    x = x + n;
	    if (isspace (*p_str) && (x % SCREEN_COLS > 0))
	    {
		x++;
	    }
	    y = y + (x / SCREEN_COLS);
	    x = x % SCREEN_COLS;
	}
    }
}
