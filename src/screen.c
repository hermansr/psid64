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

static BYTE scrtab[] = {
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, /* 0x00          */
    0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, /* 0x08          */
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, /* 0x10          */
    0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, /* 0x18          */
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, /* 0x20  !"#$%&' */
    0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, /* 0x28 ()*+,-./ */
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, /* 0x30 01234567 */
    0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, /* 0x38 89:;<=>? */
    0x00, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, /* 0x40 @ABCDEFG */
    0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, /* 0x48 HIJKLMNO */
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, /* 0x50 PQRSTUVW */
    0x58, 0x59, 0x5a, 0x1b, 0xbf, 0x1d, 0x1e, 0x64, /* 0x58 XYZ[\]^_ */
    0x27, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, /* 0x60 `abcdefg */
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, /* 0x68 hijklmno */
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, /* 0x70 pqrstuvw */
    0x18, 0x19, 0x1a, 0xbf, 0x5d, 0xbf, 0xbf, 0x20, /* 0x78 xyz{|}~  */
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, /* 0x80          */
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, /* 0x88          */
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, /* 0x90          */
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, /* 0x98          */
    0x20, 0x21, 0x03, 0x1c, 0xbf, 0x59, 0x5d, 0xbf, /* 0xa0  ¡¢£¤¥¦§ */
    0x22, 0x43, 0x01, 0x3c, 0xbf, 0x2d, 0x52, 0x63, /* 0xa8 ¨©ª«¬­®¯ */
    0x0f, 0xbf, 0x32, 0x33, 0x27, 0x15, 0xbf, 0xbf, /* 0xb0 °±²³´µ¶· */
    0x2c, 0x31, 0x0f, 0x3e, 0xbf, 0xbf, 0xbf, 0x3f, /* 0xb8 ¸¹º»¼½¾¿ */
    0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x43, /* 0xc0 ÀÁÂÃÄÅÆÇ */
    0x45, 0x45, 0x45, 0x45, 0x49, 0x49, 0x49, 0x49, /* 0xc8 ÈÉÊËÌÍÎÏ */
    0xbf, 0x4e, 0x4f, 0x4f, 0x4f, 0x4f, 0x4f, 0x18, /* 0xd0 ÐÑÒÓÔÕÖ× */
    0x4f, 0x55, 0x55, 0x55, 0x55, 0x59, 0xbf, 0xbf, /* 0xd8 ØÙÚÛÜÝÞß */
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x03, /* 0xe0 àáâãäåæç */
    0x05, 0x05, 0x05, 0x05, 0x09, 0x09, 0x09, 0x09, /* 0xe8 èéêëìíîï */
    0xbf, 0x0e, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0xbf, /* 0xf0 ðñòóôõö÷ */
    0x0f, 0x15, 0x15, 0x15, 0x15, 0x19, 0xbf, 0x19  /* 0xf8 øùúûüýþÿ */
};


/****************************************************************************
 *                       L O C A L   F U N C T I O N S                      *
 ****************************************************************************/


/****************************************************************************
 *                      G L O B A L   F U N C T I O N S                     *
 ****************************************************************************/

BYTE
iso2scr (BYTE c)
{
    return scrtab[c];
}


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
