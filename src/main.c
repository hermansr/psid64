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


/****************************************************************************
 *                           G L O B A L   D A T A                          *
 ****************************************************************************/


/****************************************************************************
 *                     L O C A L   D E F I N I T I O N S                    *
 ****************************************************************************/

#define STR_GETOPT_OPTIONS		":ho:r:vV"
#define EXIT_ERROR			1

typedef struct config_s
{
    int                     verbose;
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


static BYTE             find_driver_space (BYTE * pages, BYTE scr, BYTE chars,
					   BYTE size);
static void             find_free_space (BYTE * p_driver, BYTE * p_screen,
					 BYTE * p_chars);
static void             psid_init_driver (BYTE ** ptr, int *n,
					  BYTE driver_page, BYTE screen_page,
					  BYTE char_page);
static void             draw_screen (BYTE * p_screen);
static int              block_cmp (block_t * a, block_t * b);
static char            *build_output_filename (char *p_psid_file,
					       config_t * p_config);
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
	    if ((i - first_page) >= NUM_EXTDRV_PAGES)
	    {
		return first_page;
	    }
	    first_page = i + 1;
	}
    }

    return 0;
}


static void
find_free_space (BYTE * p_driver, BYTE * p_screen, BYTE * p_chars)
/*--------------------------------------------------------------------------*
   In          : -
   Out         : p_driver		startpage of driver, 0 means no driver
		 p_screen		startpage of screen, 0 means no screen
		 p_chars		startpage of chars, 0 means no chars
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
    else if (startp != 0xff)
    {
	/* the available pages have been specified in the PSID file */
	endp = MIN ((startp + maxp), MAX_PAGES);
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
		  BYTE char_page)
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
	vsa = (BYTE) ((screen_page & 0x3c) << 2);
	cba = (BYTE) (char_page ? (char_page >> 2) & 0x0e : 0x06);
	psid_reloc[addr++] = vsa | cba;	/* d018 */
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


static char            *
build_output_filename (char *p_psid_file, config_t * p_config)
{
    char                   *p_output_filename = NULL;
    char                   *p_str;
    int                     l;
    int                     n;

    if (p_config->p_output_filename != NULL)
    {
	p_output_filename = g_strdup (p_config->p_output_filename);
    }
    else
    {
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
	p_output_filename = g_strdup_printf ("%s%s", p_str, PRG_POSTFIX);
	g_free (p_str);
    }

    return p_output_filename;
}


static int
process_file (char *p_psid_file, config_t * p_config)
{
    int                     retval = 0;
    char                   *p_output_filename;
     /*FIXME*/ BYTE screen[SCREEN_SIZE];
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

    /* read the PSID file */
    if (p_config->verbose)
    {
	printf ("Reading PSID file `%s'\n", p_psid_file);
    }
    psid = psid_load_file (p_psid_file);
    if (!psid)
    {
	fprintf (stderr, "%s: `%s' is not a valid PSID file.\n", PACKAGE,
		 p_psid_file);
	return 1;
    }

    /* find space for driver and screen (optional) */
    find_free_space (&driver_page, &screen_page, &char_page);
    if (driver_page == 0x00)
    {
	fprintf (stderr, "%s: C64 memory has no space for driver code.\n",
		 p_psid_file);
	return 1;
    }

    /* relocate and initialize the driver */
    psid_init_driver (&psid_driver, &driver_size, driver_page, screen_page,
		      char_page);
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
	ADDRESS                 charset = char_page << 8;

	printf ("C64 memory map:\n");
	for (i = n_blocks - 1; i >= 0; i--)
	{
	    if ((charset != 0) && (blocks[i].load > charset))
	    {
		printf ("  $%04x-$%04x  Character set\n", charset,
			charset + (256 * NUM_CHAR_PAGES));
		charset = 0;
	    }
	    printf ("  $%04x-$%04x  %s\n", blocks[i].load,
		    blocks[i].load + blocks[i].size, blocks[i].p_description);
	}
	if (charset != 0)
	{
	    printf ("  $%04x-$%04x  Character set\n", charset,
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

    /* write C64 executable */
    p_output_filename = build_output_filename (p_psid_file, p_config);
    f = fopen (p_output_filename, "w");
    if (f != NULL)
    {
	if (p_config->verbose)
	{
	    printf ("Writing C64 executable `%s'\n", p_output_filename);
	}
	fwrite (psid_boot, boot_size, 1, f);
	for (i = n_blocks - 1; i >= 0; i--)
	{
	    fwrite (blocks[i].p_data, blocks[i].size, 1, f);
	}
    }
    else
    {
	fprintf (stderr, "%s: can't open output file `%s': %s\n", PACKAGE,
		 p_output_filename, g_strerror (errno));
	retval = 1;
    }
    g_free (p_output_filename);

    return retval;
}


static void
print_usage (void)
{
    printf ("Usage: %s [-hvV] [-r hvsc_root] <psid_file> <c64_file>\n",
	    PACKAGE);
}


static void
print_help (void)
{
    print_usage ();
    printf ("\n");
#ifdef HAVE_GETOPT_LONG
    printf ("  -o, --output=FILE    specify output file\n");
    printf ("  -r, --root           specify HVSC root directory\n");
    printf ("  -v, --verbose        explain what is being done\n");
    printf ("  -h, --help           display this help and exit\n");
    printf ("  -V, --version        output version information and exit\n");
#else
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
main (int argc, char **argv)
{
    config_t                config;
    int                     c;
    int                     errflg = 0;
#ifdef HAVE_GETOPT_LONG
    int                     option_index = 0;
    static struct option    long_options[] = {
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
    config.p_hvsc_root = NULL;
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
	exit (EXIT_ERROR);
    }

    while (optind < argc)
    {
	if (process_file (argv[optind++], &config))
	{
	    exit (EXIT_ERROR);
	}
    }

    return 0;
}
