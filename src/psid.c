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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "psid.h"


/****************************************************************************
 *                           G L O B A L   D A T A                          *
 ****************************************************************************/

int                     psid_errno;


/****************************************************************************
 *                     L O C A L   D E F I N I T I O N S                    *
 ****************************************************************************/

static WORD             psid_extract_word (BYTE ** buf);


/****************************************************************************
 *                            L O C A L   D A T A                           *
 ****************************************************************************/

static const char      *psid_errors[] = {
    "not enough memory",
    "could not open file",
    "PSID header not found",
    "PSID version not supported",
    "error reading PSID header",
    "SIDPLAYER MUS files not supported",
    "error reading PSID load address",
    "error reading PSID data",
    "more than 64K PSID data"
};


/****************************************************************************
 *                       L O C A L   F U N C T I O N S                      *
 ****************************************************************************/

static                  WORD
psid_extract_word (BYTE ** buf)
{
    WORD                    word = (*buf)[0] << 8 | (*buf)[1];
    *buf += 2;
    return word;
}


/****************************************************************************
 *                      G L O B A L   F U N C T I O N S                     *
 ****************************************************************************/

const char             *
psid_strerror (psid_err_t errnum)
{
    const char             *p_str;

    if ((0 <= errnum)
	&& (errnum < (sizeof (psid_errors) / sizeof (*psid_errors))))
    {
	p_str = psid_errors[errnum];
    }
    else
    {
	p_str = "unknown error number";
    }

    return p_str;
}


psid_t                 *
psid_load_file (const char *filename)
{
    psid_t                 *psid;
    FILE                   *f;
    BYTE                    buf[PSID_V2_DATA_OFFSET + 2];
    BYTE                   *ptr = buf;
    size_t                  length;

    psid = malloc (sizeof (psid_t));
    if (psid == NULL)
    {
	psid_errno = PSID_ERR_NO_MEMORY;
	return NULL;
    }

    if (!(f = fopen (filename, "r")))
    {
	free (psid);
	psid_errno = PSID_ERR_NO_FILE;
	return NULL;
    }

    if (fread (ptr, 1, 6, f) != 6 || memcmp (ptr, "PSID", 4) != 0)
    {
	psid_errno = PSID_ERR_NO_HEADER;
	goto fail;
    }


    ptr += 4;
    psid->version = psid_extract_word (&ptr);

    if (psid->version < 1 || psid->version > 2)
    {
	psid_errno = PSID_ERR_UNKNOWN_VERSION;
	goto fail;
    }

    length = (size_t) ((psid->version == 1
			? PSID_V1_DATA_OFFSET : PSID_V2_DATA_OFFSET) - 6);

    if (fread (ptr, 1, length, f) != length)
    {
	psid_errno = PSID_ERR_READ_HEADER;
	goto fail;
    }

    psid->data_offset = psid_extract_word (&ptr);
    psid->load_addr = psid_extract_word (&ptr);
    psid->init_addr = psid_extract_word (&ptr);
    psid->play_addr = psid_extract_word (&ptr);
    psid->songs = psid_extract_word (&ptr);
    psid->start_song = psid_extract_word (&ptr);
    psid->speed = psid_extract_word (&ptr) << 16;
    psid->speed |= psid_extract_word (&ptr);
    memcpy (psid->name, ptr, 32);
    psid->name[31] = '\0';
    ptr += 32;
    memcpy (psid->author, ptr, 32);
    psid->author[31] = '\0';
    ptr += 32;
    memcpy (psid->copyright, ptr, 32);
    psid->copyright[31] = '\0';
    ptr += 32;
    if (psid->version == 2)
    {
	psid->flags = psid_extract_word (&ptr);
	psid->start_page = *ptr++;
	psid->max_pages = *ptr++;
	psid->reserved = psid_extract_word (&ptr);
    }
    else
    {
	psid->flags = 0;
	psid->start_page = 0;
	psid->max_pages = 0;
	psid->reserved = 0;
    }

    /* Check for SIDPLAYER MUS files. */
    if (psid->flags & 0x01)
    {
	psid_errno = PSID_ERR_SIDPLAYER_MUS;
	goto fail;
    }

    /* Zero load address => the load address is stored in the
       first two bytes of the binary C64 data. */
    if (psid->load_addr == 0)
    {
	if (fread (ptr, 1, 2, f) != 2)
	{
	    psid_errno = PSID_ERR_LOAD_ADDRESS;
	    goto fail;
	}
	psid->load_addr = ptr[0] | ptr[1] << 8;
    }

    /* Zero init address => use load address. */
    if (psid->init_addr == 0)
    {
	psid->init_addr = psid->load_addr;
    }

    /* Read binary C64 data. */
    psid->data_size = fread (psid->data, 1, sizeof (psid->data), f);

    if (ferror (f))
    {
	psid_errno = PSID_ERR_READ_DATA;
	goto fail;
    }

    if (!feof (f))
    {
	psid_errno = PSID_ERR_TOO_MUCH_DATA;
	goto fail;
    }

    fclose (f);

    return psid;

  fail:
    fclose (f);
    free (psid);
    return NULL;
}
