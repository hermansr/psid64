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

#include "stilwrapper.h"

#include "stilview/stil.h"

static STIL             stil;

extern "C" int psid64(int argc, char **argv);


gboolean
stil_init (const char *p_hvsc_root)
/*--------------------------------------------------------------------------*
   In          : p_hvsc_root		Root directory where HVSC is located
   Out         : -
   Return value: status			success/failure
   Description : C wrapper to initialize the STIL object
   Globals     : stil			STIL object
   History     : 12-FEB-2002  RH  Initial version
 *--------------------------------------------------------------------------*/
{
    if (stil.setBaseDir (p_hvsc_root) != true)
    {
	return FALSE;
    }

    return TRUE;
}


const char                   *
stil_get_error (void)
{
    return stil.getErrorStr ();
}


gboolean
stil_get_data (const char *p_filename, char **pp_global_comment,
	       char **pp_stil_entry, char **pp_bug_entry)
/*--------------------------------------------------------------------------*
   In          : p_filename		HVSC style filename
   Out         : pp_global_comment	Global comment
		 pp_stil_entry		STIL entry
		 pp_bug_entry		Bug entry
   Return value: status			success/failure
   Description : TO_BE_DONE
   Globals     : TO_BE_DONE
   History     : 12-FEB-2002  RH  Initial version
 *--------------------------------------------------------------------------*/
{
    *pp_global_comment = NULL;
    *pp_stil_entry = NULL;
    *pp_bug_entry = NULL;

    if (stil.hasCriticalError ())
    {
	return FALSE;
    }
    *pp_global_comment = stil.getGlobalComment (p_filename);
    if (stil.hasCriticalError ())
    {
	return FALSE;
    }
    *pp_stil_entry = stil.getEntry (p_filename, 0);
    if (stil.hasCriticalError ())
    {
	return FALSE;
    }
    *pp_bug_entry = stil.getBug (p_filename, 0);
    if (stil.hasCriticalError ())
    {
	return FALSE;
    }

    return TRUE;
}


int
main (int argc, char **argv)
{
    return psid64 (argc, argv);
}
