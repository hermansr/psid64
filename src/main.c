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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "datatypes.h"
#include "psid.h"
#include "reloc65.h"
#include "screen.h"


/****************************************************************************
 *                           G L O B A L   D A T A                          *
 ****************************************************************************/


/****************************************************************************
 *                     L O C A L   D E F I N I T I O N S                    *
 ****************************************************************************/

#define STR_GETOPT_OPTIONS		":hr:vV"
#define EXIT_ERROR			1

typedef struct config_s
{
    int                     verbose;
    char                   *p_hvsc_root;
}
config_t;


#define NUM_MINDRV_PAGES		2	/* driver without screen display */
#define NUM_EXTDRV_PAGES		4	/* driver with screen display */
#define MAX_PAGES			256


#define MAX_BLOCKS			3

typedef struct block_s
{
    ADDRESS                 load;
    ADDRESS                 size;
    void                   *p_data;
    char                   *p_description;
}
block_t;

typedef int             (*SORTFUNC) (const void *, const void *);


static void             find_free_space (BYTE * p_driver, BYTE * p_screen);
static void             psid_init_driver (BYTE ** ptr, int *n,
					  BYTE driver_page, BYTE screen_page);
static void             draw_screen (BYTE * p_screen);
static int              block_cmp (block_t * a, block_t * b);
static int              process_file (char *p_psid_file, char *p_c64_file, config_t * p_config);
static void             print_usage (void);


/****************************************************************************
 *                            L O C A L   D A T A                           *
 ****************************************************************************/

static psid_t          *psid;


/****************************************************************************
 *                       L O C A L   F U N C T I O N S                      *
 ****************************************************************************/

static void
find_free_space (BYTE * p_driver, BYTE * p_screen)
/*--------------------------------------------------------------------------*
   In          : -
   Out         : p_driver		startpage of driver, 0 means no driver
		 p_screen		startpage of screen, 0 means no screen
   Return value: -
   Description : Find free space in the C64 memory map for the screen and the
		 driver code. Of course the driver code takes priority over
		 the screen.
   Globals     : psid			PSID header and data
   History     : 15-AUG-2001  RH  Initial version
 *--------------------------------------------------------------------------*/
{
    BYTE                    screens[] = {
	0x04, 0x08, 0x0c,
	0x20, 0x24, 0x28, 0x2c,
	0x30, 0x34, 0x38, 0x3c,
	0x80, 0x84, 0x88, 0x8c,
	0xa0, 0xa4, 0xa8, 0xac,
	0xb0, 0xb4, 0xb8, 0xbc,
	0x00
    };				/* possible screen locations */
    int                     startp = psid->start_page;
    int                     maxp = psid->max_pages;
    int                     i;

    *p_screen = (BYTE) (0x00);
    *p_driver = (BYTE) (0x00);

    if (startp == 0x00)
    {
	/* Used memory ranges. */
	int                     used[] = { 0x00, 0x03,
	    0xa0, 0xbf,
	    0xd0, 0xff,
	    0x00, 0x00
	};			/* calculated below */
	int                     pages[MAX_PAGES];
	int                     endp;
	int                     page;
	int                     last_page;

	startp = psid->load_addr >> 8;
	endp = (psid->load_addr + psid->data_size - 1) >> 8;

	/* Finish initialization by setting start and end pages. */
	used[6] = startp;
	used[7] = endp;

	/* Mark used pages in table. */
	memset (pages, 0, sizeof (pages));
	for (i = 0; i < sizeof (used) / sizeof (*used); i += 2)
	{
	    for (page = used[i]; page <= used[i + 1]; page++)
	    {
		pages[page] = 1;
	    }
	}

	i = 0;
	while ((*p_screen == 0x00) && (screens[i] != 0x00))
	{
	    if ((screens[i] > endp) || ((screens[i] + 4) <= startp))
	    {
		/* this screen is availabele, check if driver fits in as well */
		last_page = 0;
		page = 0;
		while ((*p_screen == 0x00) && (page < MAX_PAGES))
		{
		    if ((pages[page] != 0)
			|| ((screens[i] <= page)
			    && (page < (screens[i] + 4))))
		    {
			if ((page - last_page) >= NUM_EXTDRV_PAGES)
			{
			    *p_screen = screens[i];
			    *p_driver = last_page;
			}
			last_page = page + 1;
		    }
		    page++;
		}
	    }
	    i++;
	}

	if (*p_screen == 0x00)
	{
	    /* no suitable screen location found, but there might be enough
	       room for the driver code. */
	    last_page = 0;
	    page = 0;
	    while ((*p_driver == 0x00) && (page < MAX_PAGES))
	    {
		if (pages[page] != 0)
		{
		    if ((page - last_page) >= NUM_MINDRV_PAGES)
		    {
			*p_driver = last_page;
		    }
		    last_page = page + 1;
		}
		page++;
	    }
	}
    }
    else if (startp != 0xff)
    {
	/* the available pages have been specified in the PSID file */
	i = 0;
	while ((*p_screen == 0x00) && (screens[i] != 0x00))
	{
	    /* check if screens[i] fits into the specified area */
	    if ((startp <= screens[i])
		&& ((screens[i] + 4) <= (startp + maxp)))
	    {
		/* this screen is availabele, check if driver fits in as well */
		if ((screens[i] - startp) >= NUM_EXTDRV_PAGES)
		{
		    *p_screen = screens[i];
		    *p_driver = startp;
		}
		else if ((startp + maxp - (screens[i] + 4)) >=
			 NUM_EXTDRV_PAGES)
		{
		    *p_screen = screens[i];
		    *p_driver = screens[i] + 4;
		}
	    }
	    i++;
	}

	if ((*p_screen == 0x00) && (maxp >= NUM_MINDRV_PAGES))
	{
	    /* no suitable screen location found, but there is enough room
	       for the driver code. */
	    *p_driver = startp;
	}
    }
}


static void
psid_init_driver (BYTE ** ptr, int *n, BYTE driver_page, BYTE screen_page)
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
	fprintf (stderr, "Relocation error.");
	return;
    }

    /* Skip JMP */
    addr = 3;

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
	psid_reloc[addr++] = (BYTE) ((((screen_page & 0xc0) >> 6) ^ 3) | 0x04);	/* dd00 */
	psid_reloc[addr++] = (BYTE) (((screen_page & 0x3c) << 2) | 0x06);	/* d018 */
    }

    *ptr = psid_reloc;
    *n = psid_size;
}


static void
draw_screen (BYTE * p_screen)
{
    char                   *p_str;
    int                     i;

    screen_clear (p_screen);

    /* set title */
    screen_printf (p_screen, 5, 1, "PSID64 v%3.3s by Roland Hermans!", VERSION);

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

    /* some additional text */
    screen_wrap (p_screen, 0, 12, "\
This is an experimental PSID player for the C64. It is an \
implementation of the PSID V2 NG proposal written by Dag Lem and Simon \
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


static int
process_file (char *p_psid_file, char *p_c64_file, config_t * p_config)
{
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
    BYTE                    screen_page;
    BYTE                    driver_page;

    /* read the PSID file */
    if (p_config->verbose)
    {
	printf ("Reading PSID file '%s'\n", p_psid_file);
    }
    psid = psid_load_file (p_psid_file);
    if (!psid)
    {
	return 1;
    }

    /* find space for driver and screen (optional) */
    find_free_space (&driver_page, &screen_page);
    if (driver_page == 0x00)
    {
	fprintf (stderr, "C64 memory has no space for driver code.\n");
	return 1;
    }

    /* relocate and initialize the driver */
    psid_init_driver (&psid_driver, &driver_size, driver_page, screen_page);
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
    qsort (blocks, n_blocks, sizeof (block_t), (SORTFUNC) block_cmp);

    /* print memory map */
    if (p_config->verbose)
    {
	printf ("C64 memory map:\n");
	for (i = n_blocks - 1; i >= 0; i--)
	{
	    printf ("  $%04x-$%04x  %s\n", blocks[i].load,
		    blocks[i].load + blocks[i].size, blocks[i].p_description);
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

    psid_boot[addr++] = (BYTE) (jmp_addr & 0xff);	/* start address of driver */
    psid_boot[addr++] = (BYTE) (jmp_addr >> 8);

    /* write C64 executable */
    f = fopen (p_c64_file, "w");
    if (f != NULL)
    {
	if (p_config->verbose)
	{
	    printf ("Writing C64 executable '%s'\n", p_c64_file);
	}
	fwrite (psid_boot, boot_size, 1, f);
	for (i = n_blocks - 1; i >= 0; i--)
	{
	    fwrite (blocks[i].p_data, blocks[i].size, 1, f);
	}
    }
    else
    {
	fprintf (stderr, "could not open file %s: %s", p_c64_file,
		 strerror (errno));
	return 1;
    }

    return 0;
}


static void
print_usage (void)
{
    printf ("Usage: %s [-hvV] [-r hvsc_root] <psid_file> <c64_file>\n",
	    PACKAGE);
}


/****************************************************************************
 *                      G L O B A L   F U N C T I O N S                     *
 ****************************************************************************/

int
main (int argc, char **argv)
{
    config_t                config;
    int                     c;
    int                     errflg = 0;

    /* set default configuration */
    config.verbose = 0;
    config.p_hvsc_root = NULL;

    c = getopt (argc, argv, STR_GETOPT_OPTIONS);
    while (c != -1)
    {
	switch (c)
	{
	case 'h':
	    print_usage ();
	    exit (0);
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
	    fprintf (stderr, "Option -%c requires an argument\n", optopt);
	    errflg++;
	    break;
	case '?':
	    fprintf (stderr, "Unrecognized option: - %c\n", optopt);
	    errflg++;
	    break;
	default:
	    fprintf (stderr, "%s:%d: software error\n", __FILE__, __LINE__);
	    exit (EXIT_ERROR);
	    break;
	}
	c = getopt (argc, argv, STR_GETOPT_OPTIONS);
    }

    if ((argc - optind) != 2)
    {
	errflg++;
    }

    if (errflg)
    {
	print_usage ();
	exit (EXIT_ERROR);
    }

    return process_file (argv[optind], argv[optind + 1], &config);
}
