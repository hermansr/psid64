// SPDX-License-Identifier: GPL-2.0-or-later
/*
    psid64 - create a C64 executable from a PSID file
    Copyright (C) 2001-2003  Roland Hermans <rolandh@users.sourceforge.net>

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


    The relocating PSID driver is based on a reference implementation written
    by Dag Lem, using Andre Fachat's relocating cross assembler, xa. The
    original driver code was introduced in VICE 1.7.
*/

//////////////////////////////////////////////////////////////////////////////
//                             I N C L U D E S
//////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ConsoleApp.h"


/****************************************************************************
 *                           G L O B A L   D A T A                          *
 ****************************************************************************/


/****************************************************************************
 *                     L O C A L   D E F I N I T I O N S                    *
 ****************************************************************************/


/****************************************************************************
 *                            L O C A L   D A T A                           *
 ****************************************************************************/


/****************************************************************************
 *                       L O C A L   F U N C T I O N S                      *
 ****************************************************************************/


/****************************************************************************
 *                      G L O B A L   F U N C T I O N S                     *
 ****************************************************************************/

int
main (int argc, char **argv)
{
    int retval = 0;
    ConsoleApp app;

    if (app.main(argc, argv) == false)
    {
        retval = 1;
    }

    return retval;
}
