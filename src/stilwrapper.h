/*
    $Id$

    psid64 - create a C64 executable from a PSID file
    Copyright (C) 2001-2002  Roland Hermans <rolandh@users.sourceforge.net>

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

#ifndef STILWRAPPER_H
#define STILWRAPPER_H

/****************************************************************************
 *                             I N C L U D E S                              *
 ****************************************************************************/

#include "glib.h"


/****************************************************************************
 *                  F O R W A R D   D E C L A R A T O R S                   *
 ****************************************************************************/


/****************************************************************************
 *                     D A T A   D E C L A R A T O R S                      *
 ****************************************************************************/


/****************************************************************************
 *                  F U N C T I O N   D E C L A R A T O R S                 *
 ****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern gboolean         stil_init (const char *p_hvsc_root);
extern const char      *stil_get_error (void);
extern gboolean         stil_get_data (const char *p_filename,
				       char **pp_global_comment,
				       char **pp_stil_entry,
				       char **pp_bug_entry);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* STILWRAPPER_H */
