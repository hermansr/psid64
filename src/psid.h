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

#ifndef PSID_H
#define PSID_H

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

typedef struct psid_s
{
    /* PSID data */
    WORD                    version;
    WORD                    data_offset;
    WORD                    load_addr;
    WORD                    init_addr;
    WORD                    play_addr;
    WORD                    songs;
    WORD                    start_song;
    DWORD                   speed;
    BYTE                    name[32];
    BYTE                    author[32];
    BYTE                    copyright[32];
    WORD                    flags;
    BYTE                    start_page;
    BYTE                    max_pages;
    WORD                    reserved;
    WORD                    data_size;
    BYTE                    data[65536];
}
psid_t;

#define PSID_V1_DATA_OFFSET		0x76
#define PSID_V2_DATA_OFFSET		0x7c

#define PSID_IS_SIDPLAYER_MUS(flags)	((flags) & 0x01)
#define PSID_IS_PLAYSID_SPECIFIC(flags)	((flags) & 0x02)
#define PSID_VIDEO_STANDARD(flags)	(((flags) >> 2) & 0x03)
#define PSID_SID_MODEL(flags)		(((flags) >> 4) & 0x03)

#define PSID_VIDEO_MODE_UNKNOWN		0x00
#define PSID_VIDEO_MODE_PAL		0x01
#define PSID_VIDEO_MODE_NTSC		0x02
#define PSID_VIDEO_MODE_PAL_NTSC	0x03

#define PSID_SID_MODEL_UNKNOWN		0x00
#define PSID_SID_MODEL_MOS6581		0x01
#define PSID_SID_MODEL_MOS8580		0x02
#define PSID_SID_MODEL_MOS6581_MOS8580	0x03

typedef enum psid_err_enum
{
    PSID_ERR_NO_MEMORY,
    PSID_ERR_NO_FILE,
    PSID_ERR_NO_HEADER,
    PSID_ERR_UNKNOWN_VERSION,
    PSID_ERR_READ_HEADER,
    PSID_ERR_SIDPLAYER_MUS,
    PSID_ERR_LOAD_ADDRESS,
    PSID_ERR_READ_DATA,
    PSID_ERR_TOO_MUCH_DATA
}
psid_err_t;

extern int              psid_errno;


/****************************************************************************
 *                  F U N C T I O N   D E C L A R A T O R S                 *
 ****************************************************************************/

extern const char *psid_strerror (psid_err_t errnum);
extern psid_t *psid_load_file (const char *filename);

#endif /* PSID_H */
