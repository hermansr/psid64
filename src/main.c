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


    The relocating PSID driver is based on a reference implementation written
    by Dag Lem, using Andre Fachat's relocating cross assembler, xa. The
    original driver code was introduced in VICE 1.7.
*/

/****************************************************************************
 *                             I N C L U D E S                              *
 ****************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ctype.h>
#include <errno.h>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "glib.h"

#include "datatypes.h"
#include "psid.h"
#include "reloc65.h"
#include "screen.h"
#include "stilwrapper.h"


/****************************************************************************
 *                           G L O B A L   D A T A                          *
 ****************************************************************************/


/****************************************************************************
 *                     L O C A L   D E F I N I T I O N S                    *
 ****************************************************************************/

#define STR_GETOPT_OPTIONS		":gho:r:vV"
#define EXIT_ERROR			1

typedef struct config_s
{
    int                     verbose;
    int			    global_comment;
    char                   *p_hvsc_root;
    char                   *p_output_filename;
}
config_t;


#define NUM_MINDRV_PAGES		2	/* driver without screen display */
#define NUM_EXTDRV_PAGES		4	/* driver with screen display */
#define NUM_SCREEN_PAGES		4	/* size of screen in pages */
#define NUM_CHAR_PAGES			8	/* size of charset in pages */
#define MAX_PAGES			256	/* number of memory pages */

#define PSID_POSTFIX			".sid"
#define PRG_POSTFIX			".prg"

#define STIL_EOT_SPACES			5 /* number of spaces before EOT */

#define MAX_BLOCKS			4

typedef struct block_s
{
    ADDRESS                 load;
    ADDRESS                 size;
    void                   *p_data;
    char                   *p_description;
}
block_t;

typedef int             (*SORTFUNC) (const void *, const void *);


static BYTE             find_stil_space (BYTE * pages, BYTE scr, BYTE chars,
                                         BYTE driver, BYTE size);
static BYTE             find_driver_space (BYTE * pages, BYTE scr, BYTE chars,
					   BYTE size);
static void             find_free_space (BYTE * p_driver, BYTE * p_screen,
					 BYTE * p_chars, BYTE *p_stil,
					 BYTE stil_size);
static void             psid_init_driver (BYTE ** ptr, int *n,
					  BYTE driver_page, BYTE screen_page,
					  BYTE char_page, BYTE stil_page);
static void		add_flag (char *p_str, int *n, const char *p_flag);
static void             draw_screen (BYTE * p_screen);
static int              block_cmp (block_t * a, block_t * b);
static char            *build_output_filename (char *p_psid_file,
					       config_t * p_config);
static char            *build_hvsc_filename (const char *p_hvsc_root, const char *p_filename);
static void             get_stil_text (config_t * p_config, const char *p_psid_file, char **pp_text, int *p_text_size);
static int              process_file (char *p_psid_file, config_t * p_config);
static void             print_usage (void);
static void             print_help (void);


/****************************************************************************
 *                            L O C A L   D A T A                           *
 ****************************************************************************/

static psid_t          *psid;


/****************************************************************************
 *                       L O C A L   F U N C T I O N S                      *
 ****************************************************************************/

static                  BYTE
find_stil_space (BYTE * pages, BYTE scr, BYTE chars, BYTE driver, BYTE size)
{
    BYTE                    first_page;
    int                     i;

    first_page = 0;
    for (i = 0; i < MAX_PAGES; i++)
    {
	if (pages[i] || ((scr && (scr <= i) && (i < (scr + NUM_SCREEN_PAGES))))
	    || ((chars && (chars <= i) && (i < (chars + NUM_CHAR_PAGES))))
	    || ((driver <= i) && (i < (driver + NUM_EXTDRV_PAGES))))
	{
	    if ((i - first_page) >= size)
	    {
		return first_page;
	    }
	    first_page = i + 1;
	}
    }

    return 0;
}


static                  BYTE
find_driver_space (BYTE * pages, BYTE scr, BYTE chars, BYTE size)
{
    BYTE                    first_page;
    int                     i;

    first_page = 0;
    for (i = 0; i < MAX_PAGES; i++)
    {
	if (pages[i] || (scr && (scr <= i) && (i < (scr + NUM_SCREEN_PAGES)))
	    || (chars && (chars <= i) && (i < (chars + NUM_CHAR_PAGES))))
	{
	    if ((i - first_page) >= size)
	    {
		return first_page;
	    }
	    first_page = i + 1;
	}
    }

    return 0;
}


static void
find_free_space (BYTE * p_driver, BYTE * p_screen, BYTE * p_chars, BYTE *p_stil, BYTE stil_size)
/*--------------------------------------------------------------------------*
   In          : stil_size		size of the STIL text in pages
   Out         : p_driver		startpage of driver, 0 means no driver
		 p_screen		startpage of screen, 0 means no screen
		 p_chars		startpage of chars, 0 means no chars
		 p_stil			startpage of stil, 0 means no stil
   Return value: -
   Description : Find free space in the C64 memory map for the screen and the
		 driver code. Of course the driver code takes priority over
		 the screen.
   Globals     : psid			PSID header and data
   History     : 15-AUG-2001  RH  Initial version
		 21-SEP-2001  RH  Added support for screens located in the
				  memory ranges $4000-$8000 and $c000-$d000.
 *--------------------------------------------------------------------------*/
{
    BYTE                    pages[MAX_PAGES];
    int                     startp = psid->start_page;
    int                     maxp = psid->max_pages;
    int                     endp;
    int                     i;
    int                     j;
    int                     k;
    BYTE                    bank;
    BYTE                    scr;
    BYTE                    chars;
    BYTE                    driver;

    *p_screen = (BYTE) (0x00);
    *p_driver = (BYTE) (0x00);
    *p_chars = (BYTE) (0x00);
    *p_stil = (BYTE) (0x00);

    if (startp == 0x00)
    {
	/* Used memory ranges. */
	int                     used[] = {
	    0x00, 0x03,
	    0xa0, 0xbf,
	    0xd0, 0xff,
	    0x00, 0x00
	};			/* calculated below */

	/* Finish initialization by setting start and end pages. */
	used[6] = psid->load_addr >> 8;
	used[7] = (psid->load_addr + psid->data_size - 1) >> 8;

	/* Mark used pages in table. */
	memset (pages, 0, sizeof (pages));
	for (i = 0; i < sizeof (used) / sizeof (*used); i += 2)
	{
	    for (j = used[i]; j <= used[i + 1]; j++)
	    {
		pages[j] = 1;
	    }
	}
    }
    else if ((startp != 0xff) && (maxp != 0))
    {
	/* the available pages have been specified in the PSID file */
	endp = MIN ((startp + maxp), MAX_PAGES);

	/* check that the relocation information does not use the following
	   memory areas: 0x0000-0x03FF, 0xA000-0xBFFF and 0xD000-0xFFFF */
	if ((startp < 0x04)
	    || ((0xa0 <= startp) && (startp <= 0xbf))
	    || (startp >= 0xd0)
	    || ((endp - 1) < 0x04)
	    || ((0xa0 <= (endp - 1)) && ((endp - 1) <= 0xbf))
	    || ((endp - 1) >= 0xd0))
	{
	    return;
	}

	for (i = 0; i < MAX_PAGES; i++)
	{
	    pages[i] = ((startp <= i) && (i < endp)) ? 0 : 1;
	}
    }
    else
    {
	/* not a single page is available */
	return;
    }

    driver = 0;
    for (i = 0; i < 4; i++)
    {
	/* Calculate the VIC bank offset. Screens located inside banks 1 and 3
	   require a copy the character rom in ram. The code below uses a
	   little trick to swap bank 1 and 2 so that bank 0 and 2 are checked
	   before 1 and 3. */
	bank = (((i & 1) ^ (i >> 1)) ? i ^ 3 : i) << 6;

	for (j = 0; j < 0x40; j += 4)
	{
	    /* screen may not reside within the char rom mirror areas */
	    if (!(bank & 0x40) && (0x10 <= j) && (j < 0x20))
		continue;

	    /* check if screen area is available */
	    scr = bank + j;
	    if (pages[scr] || pages[scr + 1] || pages[scr + 2]
		|| pages[scr + 3])
		continue;

	    if (bank & 0x40)
	    {
		/* The char rom needs to be copied to RAM so let's try to find
		   a suitable location. */
		for (k = 0; k < 0x40; k += 8)
		{
		    /* char rom area may not overlap with screen area */
		    if (k == (j & 0x38))
			continue;

		    /* check if character rom area is avaiable */
		    chars = bank + k;
		    if (pages[chars] || pages[chars + 1]
			|| pages[chars + 2] || pages[chars + 3]
			|| pages[chars + 4] || pages[chars + 5]
			|| pages[chars + 6] || pages[chars + 7])
			continue;

		    driver =
			find_driver_space (pages, scr, chars,
					   NUM_EXTDRV_PAGES);
		    if (driver)
		    {
			*p_driver = driver;
			*p_screen = scr;
			*p_chars = chars;
			if (stil_size)
			{
			    *p_stil = find_stil_space (pages, scr, chars,
						       driver, stil_size);
			}
			return;
		    }
		}
	    }
	    else
	    {
		driver = find_driver_space (pages, scr, 0, NUM_EXTDRV_PAGES);
		if (driver)
		{
		    *p_driver = driver;
		    *p_screen = scr;
		    if (stil_size)
		    {
			*p_stil = find_stil_space (pages, scr, 0, driver,
						   stil_size);
		    }
		    return;
		}
	    }
	}
    }

    if (!driver)
    {
	driver = find_driver_space (pages, 0, 0, NUM_MINDRV_PAGES);
	*p_driver = driver;
    }
}


static void
psid_init_driver (BYTE ** ptr, int *n, BYTE driver_page, BYTE screen_page,
		  BYTE char_page, BYTE stil_page)
{
    BYTE                    psid_driver[] = {
#include "psiddrv.h"
    };
    BYTE                    psid_extdriver[] = {
#include "psidextdrv.h"
    };
    BYTE                   *driver;
    BYTE                   *psid_reloc;
    int                     psid_size;
    ADDRESS                 reloc_addr;
    ADDRESS                 addr;
    BYTE                    vsa;	/* video screen address */
    BYTE                    cba;	/* character memory base address */

    *ptr = NULL;
    *n = 0;

    if (!psid)
	return;

    /* select driver */
    if (screen_page == 0x00)
    {
	psid_size = sizeof (psid_driver);
	driver = psid_driver;
    }
    else
    {
	psid_size = sizeof (psid_extdriver);
	driver = psid_extdriver;
    }

    /* Relocation of C64 PSID driver code. */
    psid_reloc = malloc (psid_size);
    if (psid_reloc == NULL)
    {
	return;
    }
    memcpy (psid_reloc, driver, psid_size);
    reloc_addr = driver_page << 8;

    if (!reloc65 ((char **) &psid_reloc, &psid_size, reloc_addr))
    {
	fprintf (stderr, "%s: Relocation error.", PACKAGE);
	return;
    }

    /* Skip JMP table */
    addr = 6;

    /* Store parameters for PSID player. */
    psid_reloc[addr++] = (BYTE) (psid->start_song);
    psid_reloc[addr++] = (BYTE) (psid->songs);
    psid_reloc[addr++] = (BYTE) (psid->load_addr & 0xff);
    psid_reloc[addr++] = (BYTE) (psid->load_addr >> 8);
    psid_reloc[addr++] = (BYTE) (psid->init_addr & 0xff);
    psid_reloc[addr++] = (BYTE) (psid->init_addr >> 8);
    psid_reloc[addr++] = (BYTE) (psid->play_addr & 0xff);
    psid_reloc[addr++] = (BYTE) (psid->play_addr >> 8);
    psid_reloc[addr++] = (BYTE) (psid->speed & 0xff);
    psid_reloc[addr++] = (BYTE) ((psid->speed >> 8) & 0xff);
    psid_reloc[addr++] = (BYTE) ((psid->speed >> 16) & 0xff);
    psid_reloc[addr++] = (BYTE) (psid->speed >> 24);
    psid_reloc[addr++] = (BYTE) (psid->flags & 0x02 ? 0 : 1);
    if (screen_page != 0x00)
    {
	psid_reloc[addr++] = screen_page;
	psid_reloc[addr++] = (BYTE) ((((screen_page & 0xc0) >> 6) ^ 3) | 0x04);	/* dd00 */
	vsa = (BYTE) ((screen_page & 0x3c) << 2);
	cba = (BYTE) (char_page ? (char_page >> 2) & 0x0e : 0x06);
	psid_reloc[addr++] = vsa | cba;	/* d018 */
	psid_reloc[addr++] = stil_page;
    }

    *ptr = psid_reloc;
    *n = psid_size;
}


static void
add_flag (char *p_str, int *n, const char *p_flag)
{
    if (*n > 0)
    {
	strcat (p_str, ", ");
    }
    strcat (p_str, p_flag);
    (*n)++;
}


static void
draw_screen (BYTE * p_screen)
{
    char                   *p_str;
    char		    row[SCREEN_COLS + 1];
    int                     i;

    screen_clear (p_screen);

    /* set title */
    screen_printf (p_screen, 5, 1, "PSID64 v%3.3s by Roland Hermans!",
		   VERSION);

    /* characters for color line effect */
    p_screen[4] = 0x70;
    p_screen[35] = 0x6e;
    p_screen[44] = 0x5d;
    p_screen[75] = 0x5d;
    p_screen[84] = 0x6d;
    p_screen[115] = 0x7d;
    for (i = 0; i < 30; i++)
    {
	p_screen[5 + i] = 0x40;
	p_screen[85 + i] = 0x40;
    }

    /* information lines */
    screen_printf (p_screen, 0, 4, "Name     : %-29.29s", psid->name);
    screen_printf (p_screen, 0, 5, "Author   : %-29.29s", psid->author);
    screen_printf (p_screen, 0, 6, "Copyright: %-29.29s", psid->copyright);
    screen_printf (p_screen, 0, 7, "Load     : $%04X-$%04X",
		   psid->load_addr, psid->load_addr + psid->data_size);
    screen_printf (p_screen, 0, 8, "Init     : $%04X", psid->init_addr);
    if (psid->play_addr)
    {
	p_str = g_strdup_printf ("$%04X", psid->play_addr);
    }
    else
    {
	p_str = g_strdup ("N/A");
    }
    screen_printf (p_screen, 0, 9, "Play     : %s", p_str);
    g_free (p_str);
    if (psid->songs > 1)
    {
	p_str = g_strdup_printf (" (start with %d)", psid->start_song);
    }
    else
    {
	p_str = g_strdup ("");
    }
    screen_printf (p_screen, 0, 10, "Tunes    : %d%s", psid->songs, p_str);
    g_free (p_str);

    i = 0;
    sprintf (row, "Flags    : ");
    if (PSID_IS_PLAYSID_SPECIFIC (psid->flags) != 0)
    {
	add_flag (row, &i, "PlaySID");
    }
    switch (PSID_VIDEO_STANDARD (psid->flags))
    {
    case PSID_VIDEO_MODE_PAL:
	add_flag (row, &i, "PAL");
	break;
    case PSID_VIDEO_MODE_NTSC:
	add_flag (row, &i, "NTSC");
	break;
    case PSID_VIDEO_MODE_PAL_NTSC:
	add_flag (row, &i, "PAL/NTSC");
        break;
    default:
        break;
    }
    switch (PSID_SID_MODEL (psid->flags))
    {
    case PSID_SID_MODEL_MOS6581:
	add_flag (row, &i, "6581");
	break;
    case PSID_SID_MODEL_MOS8580:
	add_flag (row, &i, "8580");
	break;
    case PSID_SID_MODEL_MOS6581_MOS8580:
	add_flag (row, &i, "6581/8580");
	break;
    default:
        break;
    }
    if (i == 0)
    {
	add_flag (row, &i, "-");
    }
    screen_printf (p_screen, 0, 11, "%s", row);

    /* some additional text */
    screen_wrap (p_screen, 0, 13, "\
This is an experimental PSID player for the C64. It is an \
implementation of the PSID V2 NG standard written by Dag Lem and Simon \
White. The driver code and screen are relocated based on information \
stored inside the PSID.");

    /* flashing bottom line (should be exactly 38 characters) */
    screen_printf (p_screen, 1, 24, "Website: http://psid64.sourceforge.net");
}


static int
block_cmp (block_t * a, block_t * b)
{
    if (a->load > b->load)
    {
	return -1;
    }
    if (a->load < b->load)
    {
	return 1;
    }

    return 0;
}


static char            *
build_output_filename (char *p_psid_file, config_t * p_config)
{
    char                   *p_output_filename = NULL;
    char                   *p_str;
    int                     l;
    int                     n;

    if (p_config->p_output_filename != NULL)
    {
	/* use filename specified by --output option */
	p_output_filename = g_strdup (p_config->p_output_filename);
    }
    else
    {
	/* strip .psid extension */
	p_str = g_basename (p_psid_file);
	l = (p_str != NULL) ? strlen (p_str) : 0;
	n = strlen (PSID_POSTFIX);
	if ((l >= n) && (g_strcasecmp (p_str + l - n, PSID_POSTFIX) == 0))
	{
	    p_str = g_strndup (p_str, l - n);
	}
	else
	{
	    p_str = g_strdup (p_str);
	}

	/* add .prg extension */
	p_output_filename = g_strdup_printf ("%s%s", p_str, PRG_POSTFIX);

	g_free (p_str);
    }

    return p_output_filename;
}


static char            *
build_hvsc_filename (const char *p_hvsc_root, const char *p_filename)
{
    char                   *p_pos;

    p_pos = strstr (p_filename, p_hvsc_root);
    if (p_pos != NULL)
    {
	return p_pos + strlen (p_hvsc_root);
    }

    return (char *) p_filename;
}


static void
get_stil_text (config_t * p_config, const char *p_psid_file,
               char **pp_text, int *p_text_size)
{
    char *p_filename;
    char *global_comment = NULL;
    char *stil_entry = NULL;
    char *bug_entry = NULL;
    char *p_src;
    char *p_dest;
    int space;
    int i;

    /* initialize output parameters */
    *pp_text = NULL;
    *p_text_size = 0;

    p_filename = build_hvsc_filename (p_config->p_hvsc_root, p_psid_file);

    if (stil_get_data(p_filename, &global_comment, &stil_entry, &bug_entry) == TRUE)
    {
	if (p_config->global_comment == 0)
	{
	    global_comment = NULL;
	}
	*pp_text = g_strdup_printf ("%s%s%s%*s", 
				    global_comment ? global_comment : "",
				    stil_entry ? stil_entry : "",
				    bug_entry ? bug_entry : "",
				    STIL_EOT_SPACES, "");

	/* remove all double whitespace characters from the message */
	p_src = *pp_text;
	p_dest = *pp_text;
	space = 0;
	while (*p_src) {
		if (isspace(*p_src))
		{
		    space++;
		    p_src++;
		}
		else
		{
		    if (space) {
		       *(p_dest++) = iso2scr(' ');
		       space = 0;
		    }
		    *(p_dest++) = iso2scr(*(p_src++));
		}
	}

	/* check if the message contained at least one graphical character */
	if (p_dest != *pp_text)
	{
	    /* pad the scroll text with some space characters */
	    for (i = 0; i < STIL_EOT_SPACES; i++)
	    {
		*(p_dest++) = iso2scr(' ');
	    }
	    *p_dest = 0xff;

	    /* calculcate length of STIL scroll text (including EOT marker) */
	    *p_text_size = (p_dest - *pp_text) + 1;
        }
    }
}


static int
process_file (char *p_psid_file, config_t * p_config)
{
    int                     retval = 0;
    char                   *p_output_filename = NULL; /* to prevent warning */
    char                   *p_stil_text;
    int                     stil_text_size;
    BYTE                    screen[SCREEN_SIZE];
    block_t                 blocks[MAX_BLOCKS];
    int                     n_blocks;
    int                     i;
    FILE                   *f;
    BYTE                   *psid_driver;
    int                     driver_size;
    BYTE                    psid_boot[] = {
#include "psidboot.h"
    };
    ADDRESS                 boot_size = sizeof (psid_boot);
    ADDRESS                 jmp_addr;
    ADDRESS                 addr;
    ADDRESS                 eof;
    ADDRESS                 size;
    BYTE                    driver_page;
    BYTE                    screen_page;
    BYTE                    char_page;
    BYTE		    stil_page;
    BYTE                    stil_page_size;

    /* read the PSID file */
    if (p_config->verbose)
    {
	fprintf (stderr, "Reading file `%s'\n", p_psid_file);
    }
    psid = psid_load_file (p_psid_file);
    if (!psid)
    {
	fprintf (stderr, "%s: `%s' is not a valid PSID or RSID file.\n", PACKAGE,
		 p_psid_file);
	return 1;
    }

    if (p_config->p_hvsc_root != NULL)
    {
	/* retrieve STIL entry for this PSID file */
	get_stil_text (p_config, p_psid_file, &p_stil_text,
	               &stil_text_size);
	stil_page_size = (stil_text_size > 0) ? ((stil_text_size + 255) >> 8) : 0;
    }
    else
    {
	/* STIL is disabled or not available */
	p_stil_text = NULL;
	stil_page_size = (BYTE) 0x00;
    }

    /* checks according to PSID V2 NG programmers implementation guide */
    if ((psid->start_page != 0x00) && (psid->start_page != 0xff) &&
        (psid->max_pages > 0))
    {
	int                 load_start = psid->load_addr;
	int                 load_end = psid->load_addr + psid->data_size;
	int                 free_start = psid->start_page << 8;
	int                 free_end = (psid->start_page + psid->max_pages) << 8;

	/* Relocation information partially covering or encompassing the entire
	   the tune's load image is not allowed. Note that there is no need to
	   clip either load_end or free_end here (both load_start and free_start
	   are always below 0x10000). */
	if ((load_end > free_start) && (load_start < free_end))
	{
	    fprintf (stderr, "%s: Bad PSID header: relocation information overlaps the load image.\n",
		     p_psid_file);
	    return 1;
	}
    }

    /* find space for driver and screen (optional) */
    find_free_space (&driver_page, &screen_page, &char_page, &stil_page, stil_page_size);
    if (driver_page == 0x00)
    {
	fprintf (stderr, "%s: C64 memory has no space for driver code.\n",
		 p_psid_file);
	return 1;
    }

    /* relocate and initialize the driver */
    psid_init_driver (&psid_driver, &driver_size, driver_page, screen_page,
		      char_page, stil_page);
    jmp_addr = driver_page << 8;

    /* fill the blocks structure */
    n_blocks = 0;
    blocks[n_blocks].load = driver_page << 8;
    blocks[n_blocks].size = driver_size;
    blocks[n_blocks].p_data = psid_driver;
    blocks[n_blocks].p_description = "Driver code";
    n_blocks++;
    blocks[n_blocks].load = psid->load_addr;
    blocks[n_blocks].size = psid->data_size;
    blocks[n_blocks].p_data = psid->data;
    blocks[n_blocks].p_description = "Music data";
    n_blocks++;
    if (screen_page != 0x00)
    {
	draw_screen (screen);
	blocks[n_blocks].load = screen_page << 8;
	blocks[n_blocks].size = sizeof (screen);
	blocks[n_blocks].p_data = screen;
	blocks[n_blocks].p_description = "Screen";
	n_blocks++;
    }
    if (stil_page != 0x00)
    {
	blocks[n_blocks].load = stil_page << 8;
	blocks[n_blocks].size = stil_text_size;
	blocks[n_blocks].p_data = p_stil_text;
	blocks[n_blocks].p_description = "STIL text";
	n_blocks++;
    }
    qsort (blocks, n_blocks, sizeof (block_t), (SORTFUNC) block_cmp);

    /* print memory map */
    if (p_config->verbose)
    {
	ADDRESS                 charset = char_page << 8;

	fprintf (stderr, "C64 memory map:\n");
	for (i = n_blocks - 1; i >= 0; i--)
	{
	    if ((charset != 0) && (blocks[i].load > charset))
	    {
		fprintf (stderr, "  $%04x-$%04x  Character set\n", charset,
			 charset + (256 * NUM_CHAR_PAGES));
		charset = 0;
	    }
	    fprintf (stderr, "  $%04x-$%04x  %s\n", blocks[i].load,
		     blocks[i].load + blocks[i].size, blocks[i].p_description);
	}
	if (charset != 0)
	{
	    fprintf (stderr, "  $%04x-$%04x  Character set\n", charset,
		     charset + (256 * NUM_CHAR_PAGES));
	}
    }

    /* calculate total size of the blocks */
    size = 0;
    for (i = 0; i < n_blocks; i++)
    {
	size = size + blocks[i].size;
    }
    /* the value 0x0801 is related to start of the code in psidboot.a65 */
    eof = 0x0801 + boot_size - 2 + size;
    addr = 17;
    psid_boot[addr++] = (BYTE) (eof & 0xff);	/* end of C64 file */
    psid_boot[addr++] = (BYTE) (eof >> 8);
    psid_boot[addr++] = (BYTE) (0x10000 & 0xff);	/* end of high memory */
    psid_boot[addr++] = (BYTE) (0x10000 >> 8);
    psid_boot[addr++] = (BYTE) ((size + 0xff) >> 8);	/* number of pages to copy */
    psid_boot[addr++] = (BYTE) ((0x10000 - size) & 0xff);	/* start of blocks after moving */
    psid_boot[addr++] = (BYTE) ((0x10000 - size) >> 8);
    psid_boot[addr++] = (BYTE) (n_blocks - 1);	/* number of blocks - 1 */

    /* copy block data to psidboot.a65 paramters */
    for (i = 0; i < n_blocks; i++)
    {
	psid_boot[addr + i] = (BYTE) (blocks[i].load & 0xff);
	psid_boot[addr + MAX_BLOCKS + i] = (BYTE) (blocks[i].load >> 8);
	psid_boot[addr + 2 * MAX_BLOCKS + i] = (BYTE) (blocks[i].size & 0xff);
	psid_boot[addr + 3 * MAX_BLOCKS + i] = (BYTE) (blocks[i].size >> 8);
    }
    addr = addr + 4 * MAX_BLOCKS;

    psid_boot[addr++] = (BYTE) (char_page);	/* page for character set, or 0 */
    psid_boot[addr++] = (BYTE) (jmp_addr & 0xff);	/* start address of driver */
    psid_boot[addr++] = (BYTE) (jmp_addr >> 8);
    psid_boot[addr++] = (BYTE) ((jmp_addr+3) & 0xff);	/* address of new stop vector */
    psid_boot[addr++] = (BYTE) ((jmp_addr+3) >> 8);	/* for tunes that call $a7ae during init */

    /* write C64 executable */
    if ((p_config->p_output_filename != NULL)
        && (strcmp (p_config->p_output_filename, "-") == 0))
    {
	f = stdout;
    }
    else
    {
	p_output_filename = build_output_filename (p_psid_file, p_config);
	f = fopen (p_output_filename, "wb");
    }
    if (f != NULL)
    {
	if (p_config->verbose)
	{
	    fprintf (stderr, "Writing C64 executable ");
	    if (f != stdout)
	    {
		fprintf (stderr, " `%s'\n", p_output_filename);
	    }
	    else
	    {
		fprintf (stderr, " to standard output\n");
	    }
	}
	fwrite (psid_boot, boot_size, 1, f);
	for (i = n_blocks - 1; i >= 0; i--)
	{
	    fwrite (blocks[i].p_data, blocks[i].size, 1, f);
	}
	if (f != stdout)
	{
	    fclose (f);
	}
    }
    else
    {
	fprintf (stderr, "%s: can't open output file `%s': %s\n", PACKAGE,
		 p_output_filename, g_strerror (errno));
	retval = 1;
    }
    if (f != stdout)
    {
	g_free (p_output_filename);
    }

    g_free (p_stil_text);

    return retval;
}


static void
print_usage (void)
{
    printf ("Usage: %s [OPTION]... PSID_FILE...\n",
	    PACKAGE);
}


static void
print_help (void)
{
    print_usage ();
    printf ("\n");
#ifdef HAVE_GETOPT_LONG
    printf ("  -g, --global-comment include the global comment STIL text\n");
    printf ("  -o, --output=FILE    specify output file\n");
    printf ("  -r, --root           specify HVSC root directory\n");
    printf ("  -v, --verbose        explain what is being done\n");
    printf ("  -h, --help           display this help and exit\n");
    printf ("  -V, --version        output version information and exit\n");
#else
    printf ("  -g                   include the global comment STIL text\n");
    printf ("  -o                   specify output file\n");
    printf ("  -r                   specify HVSC root directory\n");
    printf ("  -v                   explain what is being done\n");
    printf ("  -h                   display this help and exit\n");
    printf ("  -V                   output version information and exit\n");
#endif
    printf ("\nReport bugs to <rolandh@users.sourceforge.net>.\n");
}


/****************************************************************************
 *                      G L O B A L   F U N C T I O N S                     *
 ****************************************************************************/

int
psid64 (int argc, char **argv)
{
    config_t                config;
    int                     c;
    int                     errflg = 0;
#ifdef HAVE_GETOPT_LONG
    int                     option_index = 0;
    static struct option    long_options[] = {
	{"global-comment", 0, NULL, 'g'},
	{"help", 0, NULL, 'h'},
	{"output", 1, NULL, 'o'},
	{"root", 1, NULL, 'r'},
	{"verbose", 0, NULL, 'v'},
	{"version", 0, NULL, 'V'},
	{NULL, 0, NULL, 0}
    };
#endif

    /* set default configuration */
    config.verbose = 0;
    config.global_comment = 0;
    config.p_hvsc_root = getenv ("HVSC_BASE");
    config.p_output_filename = NULL;

#ifdef HAVE_GETOPT_LONG
    c = getopt_long (argc, argv, STR_GETOPT_OPTIONS, long_options,
		     &option_index);
#else
    c = getopt (argc, argv, STR_GETOPT_OPTIONS);
#endif
    while (c != -1)
    {
	switch (c)
	{
	case 'g':
	    config.global_comment = 1;
	    break;
	case 'h':
	    print_help ();
	    exit (0);
	    break;
	case 'o':
	    config.p_output_filename = optarg;
	    break;
	case 'r':
	    config.p_hvsc_root = optarg;
	    break;
	case 'v':
	    config.verbose = 1;
	    break;
	case 'V':
	    printf ("%s version %s\n", PACKAGE, VERSION);
	    exit (0);
	    break;
	case ':':
	    fprintf (stderr, "%s: option requires an argument -- %c\n",
		     PACKAGE, optopt);
	    errflg++;
	    break;
	case '?':
	    fprintf (stderr, "%s: invalid option -- %c\n", PACKAGE, optopt);
	    errflg++;
	    break;
	default:
	    fprintf (stderr, "%s:%s:%d: software error\n", PACKAGE, __FILE__,
		     __LINE__);
	    exit (EXIT_ERROR);
	    break;
	}
#ifdef HAVE_GETOPT_LONG
	c = getopt_long (argc, argv, STR_GETOPT_OPTIONS, long_options,
			 &option_index);
#else
	c = getopt (argc, argv, STR_GETOPT_OPTIONS);
#endif
    }

    if ((argc - optind) < 1)
    {
	print_usage ();
	errflg++;
    }

    if (errflg)
    {
	fprintf (stderr, "Try `%s --help' for more information.\n", PACKAGE);
	return (EXIT_ERROR);
    }

    /* initialize stilview object by setting the HVSC root directory */
    if ((config.p_hvsc_root != NULL)
        && (stil_init(config.p_hvsc_root) != TRUE))
    {
	fprintf (stderr, "%s: STILView will be disabled\n", stil_get_error());
	config.p_hvsc_root = NULL;
    }

    while (optind < argc)
    {
	if (process_file (argv[optind++], &config))
	{
	    return (EXIT_ERROR);
	}
    }

    return 0;
}
