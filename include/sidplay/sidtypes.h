// SPDX-License-Identifier: GPL-2.0-or-later
/***************************************************************************
                          sidtypes.h  -  type definition file
                             -------------------
    begin                : Mon Jul 3 2000
    copyright            : (C) 2000 by Simon White
    email                : s_a_white@email.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _sidtypes_h_
#define _sidtypes_h_

#include "sidint.h"
#include "sidconfig.h"

#if SID_SIZEOF_CHAR == 1
#   if (SID_SIZEOF_SHORT_INT == 2) || (SID_SIZEOF_INT == 2)
#       if (SID_SIZEOF_INT == 4) || (SID_SIZEOF_LONG_INT == 4)
//#           define SID_OPTIMISE_MEMORY_ACCESS
#       endif
#   endif
#endif

#if SID_SIZEOF_CHAR != 1
#   error Code cannot work correctly on this platform as no real 8 bit data type supported!
#endif

#ifndef SID_HAVE_BOOL
#   ifdef SID_HAVE_STDBOOL_H
#       include <stdbool.h>
#   else
        typedef int   bool;
#       define  true  1
#       define  false 0
#   endif /* SID_HAVE_STDBOOL_H */
#endif /* SID_HAVE_BOOL */

/* Custom types */

typedef unsigned int uint;

#endif /* _sidtypes_h_ */
