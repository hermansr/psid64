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

#ifndef SCREEN_H
#define SCREEN_H

/****************************************************************************
 *                             I N C L U D E S                              *
 ****************************************************************************/

#include "datatypes.h"


/****************************************************************************
 *                  F O R W A R D   D E C L A R A T O R S                   *
 ****************************************************************************/


/****************************************************************************
 *                     D A T A   D E C L A R A T O R S                      *
 ****************************************************************************/

#define SCREEN_ROWS			25
#define SCREEN_COLS			40
#define SCREEN_SIZE			(SCREEN_COLS * SCREEN_ROWS * sizeof(BYTE))


/****************************************************************************
 *                  F U N C T I O N   D E C L A R A T O R S                 *
 ****************************************************************************/

extern void screen_clear (BYTE *p_screen);
extern void screen_printf (BYTE *p_screen, int x, int y, char *p_format, ...);
extern void screen_wrap (BYTE *p_screen, int x, int y, char *p_str);


#endif /* SCREEN_H */
