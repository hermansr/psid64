/*
    $Id$

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
*/

//////////////////////////////////////////////////////////////////////////////
//                             I N C L U D E S
//////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cctype>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>

#include <psid64/psid64.h>

#include "reloc65.h"
#include "screen.h"
#include "stilview/stil.h"
#include "exomizer/exomizer.h"

using std::cerr;
using std::dec;
using std::endl;
using std::hex;
using std::ofstream;
using std::ostream;
using std::ostringstream;
using std::setfill;
using std::setw;
using std::string;
using std::uppercase;


//////////////////////////////////////////////////////////////////////////////
//                     L O C A L   D E F I N I T I O N S
//////////////////////////////////////////////////////////////////////////////

typedef int             (*SORTFUNC) (const void*, const void*);

#if defined(HAVE_IOS_OPENMODE)
    typedef std::ios::openmode openmode;
#else
    typedef int openmode;
#endif

static inline unsigned int min(unsigned int a, unsigned int b);
static int block_cmp(block_t* a, block_t* b);


//////////////////////////////////////////////////////////////////////////////
//                       L O C A L   F U N C T I O N S
//////////////////////////////////////////////////////////////////////////////

static inline unsigned int
min(unsigned int a, unsigned int b)
{
    if (a <= b)
    {
	return a;
    }
    return b;
}


static int
block_cmp(block_t* a, block_t* b)
{
    if (a->load < b->load)
    {
	return -1;
    }
    if (a->load > b->load)
    {
	return 1;
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////////
//                   P R I V A T E   M E M B E R   D A T A
//////////////////////////////////////////////////////////////////////////////

const char* Psid64::txt_relocOverlapsImage = "PSID64: relocation information overlaps the load image";
const char* Psid64::txt_notEnoughC64Memory = "PSID64: C64 memory has no space for driver code";
const char* Psid64::txt_fileIoError = "PSID64: File I/O error";
const char* Psid64::txt_noSidTuneLoaded = "PSID64: No SID tune loaded";
const char* Psid64::txt_noSidTuneConverted = "PSID64: No SID tune converted";


//////////////////////////////////////////////////////////////////////////////
//               P U B L I C   M E M B E R   F U N C T I O N S
//////////////////////////////////////////////////////////////////////////////

// constructor

Psid64::Psid64() :
    m_blankScreen(false),
    m_compress(false),
    m_initialSong(0),
    m_useGlobalComment(false),
    m_verbose(false),
    m_tune(0),
    m_stil(new STIL),
    m_screen(new Screen),
    m_programData(0),
    m_programSize(0)
{
}

// destructor

Psid64::~Psid64()
{
   delete m_stil;
   delete m_screen;
   delete[] m_programData;
}


bool Psid64::setHvscRoot(std::string &hvscRoot)
{
    m_hvscRoot = hvscRoot;
    if (!m_hvscRoot.empty())
    {
	if (!m_stil->setBaseDir(m_hvscRoot.c_str()))
	{
	    m_statusString = m_stil->getErrorStr();
	    return false;
	}
    }

    return true;
}


bool Psid64::setDatabaseFileName(std::string &databaseFileName)
{
    m_databaseFileName = databaseFileName;
    if (!m_databaseFileName.empty())
    {
        if (m_database.open(m_databaseFileName.c_str()) < 0)
        {
	    m_statusString = m_database.error();
	    return false;
        }
    }

    return true;
}


bool
Psid64::load(const char* fileName)
{
    if (!m_tune.load(fileName))
    {
        m_fileName.clear();
	m_statusString = m_tune.getInfo().statusString;
	return false;
    }

    m_tune.getInfo(m_tuneInfo);

    m_fileName = fileName;

    return true;
}


bool
Psid64::convert()
{
    static const uint_least8_t psid_boot[] = {
#include "psidboot.h"
    };
    block_t blocks[MAX_BLOCKS];
    int numBlocks;
    uint_least8_t* psid_mem;
    uint_least8_t* psid_driver;
    int driver_size;
    uint_least16_t boot_size = sizeof(psid_boot);
    uint_least16_t size;

    // ensure valid sidtune object
    if (!m_tune)
    {
	m_statusString = txt_noSidTuneLoaded;
	return false;
    }

    // handle special treatment of tunes programmed in BASIC
    if (m_tuneInfo.compatibility == SIDTUNE_COMPATIBILITY_BASIC) {
	return convertBASIC();
    }

    // retrieve STIL entry for this SID tune
    if (!formatStilText())
    {
	return false;
    }

    // find space for driver and screen (optional)
    findFreeSpace();
    if (m_driverPage == 0x00)
    {
	m_statusString = txt_notEnoughC64Memory;
	return false;
    }

    // use minimal driver if screen blanking is enabled
    if (m_blankScreen)
    {
	m_screenPage = (uint_least8_t) 0x00;
	m_charPage = (uint_least8_t) 0x00;
	m_stilPage = (uint_least8_t) 0x00;
    }

    // relocate and initialize the driver
    initDriver (&psid_mem, &psid_driver, &driver_size);

    // fill the blocks structure
    numBlocks = 0;
    blocks[numBlocks].load = m_driverPage << 8;
    blocks[numBlocks].size = driver_size;
    blocks[numBlocks].data = psid_driver;
    blocks[numBlocks].description = "Driver code";
    ++numBlocks;

    blocks[numBlocks].load = m_tuneInfo.loadAddr;
    blocks[numBlocks].size = m_tuneInfo.c64dataLen;
    uint_least8_t c64buf[65536];
    m_tune.placeSidTuneInC64mem(c64buf);
    blocks[numBlocks].data = &(c64buf[m_tuneInfo.loadAddr]);
    blocks[numBlocks].description = "Music data";
    ++numBlocks;

    if (m_screenPage != 0x00)
    {
	drawScreen();
	blocks[numBlocks].load = m_screenPage << 8;
	blocks[numBlocks].size = m_screen->getDataSize();
	blocks[numBlocks].data = m_screen->getData();
	blocks[numBlocks].description = "Screen";
	++numBlocks;
    }

    if (m_stilPage != 0x00)
    {
	blocks[numBlocks].load = m_stilPage << 8;
	blocks[numBlocks].size = m_stilText.length();
	blocks[numBlocks].data = (uint_least8_t*) m_stilText.c_str();
	blocks[numBlocks].description = "STIL text";
	++numBlocks;
    }

    qsort(blocks, numBlocks, sizeof(block_t), (SORTFUNC) block_cmp);

    // print memory map
    if (m_verbose)
    {
	uint_least16_t charset = m_charPage << 8;

	cerr << "C64 memory map:" << endl;
	for (int i = 0; i < numBlocks; ++i)
	{
	    if ((charset != 0) && (blocks[i].load > charset))
	    {
		cerr << "  $" << toHexWord(charset) << "-$"
		     << toHexWord(charset + (256 * NUM_CHAR_PAGES))
		     << "  Character set" << endl;
		charset = 0;
	    }
	    cerr << "  $" << toHexWord(blocks[i].load) << "-$"
	         << toHexWord(blocks[i].load + blocks[i].size) << "  "
		 << blocks[i].description << endl;
	}
	if (charset != 0)
	{
	    cerr << "  $" << toHexWord(charset) << "-$"
		 << toHexWord(charset + (256 * NUM_CHAR_PAGES))
		 << "  Character set" << endl;
	}
    }

    // calculate total size of the blocks
    size = 0;
    for (int i = 0; i < numBlocks; ++i)
    {
	size = size + blocks[i].size;
    }

    m_programSize = boot_size + size;
    delete[] m_programData;
    m_programData = new uint_least8_t[m_programSize];
    uint_least8_t *dest = m_programData;
    memcpy(dest, psid_boot, boot_size);

    // the value 0x0801 is related to start of the code in psidboot.a65
    uint_least16_t addr = 19;

    // fill in the initial song number (passed in at boot time)
    uint_least16_t song;
    song  = dest[addr];
    song += (uint_least16_t) dest[addr + 1] << 8;
    song -= (0x0801 - 2);
    int initialSong;
    if ((1 <= m_initialSong) && (m_initialSong <= m_tuneInfo.songs))
    {
	initialSong = m_initialSong;
    }
    else
    {
	initialSong = m_tuneInfo.startSong;
    }
    dest[song] = (uint_least8_t) ((initialSong - 1) & 0xff);

    uint_least16_t eof = 0x0801 + boot_size - 2 + size;
    dest[addr++] = (uint_least8_t) (eof & 0xff); // end of C64 file
    dest[addr++] = (uint_least8_t) (eof >> 8);
    dest[addr++] = (uint_least8_t) (0x10000 & 0xff); // end of high memory
    dest[addr++] = (uint_least8_t) (0x10000 >> 8);
    dest[addr++] = (uint_least8_t) ((size + 0xff) >> 8); // number of pages to copy
    dest[addr++] = (uint_least8_t) ((0x10000 - size) & 0xff); // start of blocks after moving
    dest[addr++] = (uint_least8_t) ((0x10000 - size) >> 8);
    dest[addr++] = (uint_least8_t) (numBlocks - 1); // number of blocks - 1
    dest[addr++] = (uint_least8_t) (m_charPage); // page for character set, or 0
    uint_least16_t jmpAddr = m_driverPage << 8;
    dest[addr++] = (uint_least8_t) (jmpAddr & 0xff); // start address of driver
    dest[addr++] = (uint_least8_t) (jmpAddr >> 8);
    dest[addr++] = (uint_least8_t) ((jmpAddr+3) & 0xff); // address of new stop vector
    dest[addr++] = (uint_least8_t) ((jmpAddr+3) >> 8); // for tunes that call $a7ae during init

    // copy block data to psidboot.a65 parameters
    for (int i = 0; i < numBlocks; ++i)
    {
	uint_least16_t offs = addr + numBlocks - 1 - i;
	dest[offs] = (uint_least8_t) (blocks[i].load & 0xff);
	dest[offs + MAX_BLOCKS] = (uint_least8_t) (blocks[i].load >> 8);
	dest[offs + 2 * MAX_BLOCKS] = (uint_least8_t) (blocks[i].size & 0xff);
	dest[offs + 3 * MAX_BLOCKS] = (uint_least8_t) (blocks[i].size >> 8);
    }
    addr = addr + 4 * MAX_BLOCKS;
    dest += boot_size;

    // copy blocks to c64 program file
    for (int i = 0; i < numBlocks; ++i)
    {
	memcpy(dest, blocks[i].data, blocks[i].size);
	dest += blocks[i].size;
    }

    // free memory of relocated driver
    delete[] psid_mem;

#if 0
    // FIXME: retrieve song length database information
    for (int i = 0; i < m_tuneInfo.songs; ++i)
    {
	m_tune.selectSong(i + 1);
        int_least32_t length = m_database.length (m_tune);
	if (m_verbose && (i > 0))
	{
	    cerr << i + 1 << ": " << length << endl;
	}
    }
#endif

    if (m_compress)
    {
	// Use Exomizer to compress the program data. The first two bytes
	// of m_programData are skipped as these contain the load address.
	int load = 0x0801;
	int start = 0x080d;
	uint_least8_t* compressedData = new uint_least8_t[0x10000];
	m_programSize = exomizer(m_programData + 2, m_programSize - 2, load, start, compressedData);
	delete[] m_programData;
	m_programData = compressedData;
    }

    return true;
}


bool
Psid64::save(const char* fileName)
{
    // Open binary output file stream.
    openmode createAttr = std::ios::out;
#if defined(HAVE_IOS_BIN)
    createAttr |= std::ios::bin;
#else
    createAttr |= std::ios::binary;
#endif

    ofstream outfile(fileName, createAttr);
    return write(outfile);
}


bool
Psid64::write(ostream& out)
{
    if (!m_programData)
    {
	m_statusString = txt_noSidTuneConverted;
	return false;
    }

    out.write((const char*) m_programData, m_programSize);
    if (!out)
    {
	m_statusString = txt_fileIoError;
	return false;
    }

    return true;
}


//////////////////////////////////////////////////////////////////////////////
//            P R O T E C T E D   M E M B E R   F U N C T I O N S
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//              P R I V A T E   M E M B E R   F U N C T I O N S
//////////////////////////////////////////////////////////////////////////////

bool
Psid64::convertBASIC()
{
    // allocate space for BASIC program
    m_programSize = 2 + m_tuneInfo.c64dataLen;
    delete[] m_programData;
    m_programData = new uint_least8_t[m_programSize];

    // first the load address
    m_programData[0] = (uint_least8_t) (m_tuneInfo.loadAddr & 0xff);
    m_programData[1] = (uint_least8_t) (m_tuneInfo.loadAddr >> 8);

    // then copy the BASIC program
    uint_least8_t c64buf[65536];
    m_tune.placeSidTuneInC64mem(c64buf);
    memcpy(m_programData + 2, &(c64buf[m_tuneInfo.loadAddr]), m_tuneInfo.c64dataLen);

    // print memory map
    if (m_verbose)
    {
	cerr << "C64 memory map:" << endl;
	cerr << "  $" << toHexWord(m_tuneInfo.loadAddr) << "-$"
	     << toHexWord(m_tuneInfo.loadAddr +m_tuneInfo.c64dataLen) << "  "
	     << "BASIC program" << endl;
    }

    return true;
}


bool
Psid64::formatStilText()
{
    m_stilText.clear();

    if (m_hvscRoot.empty())
    {
	return true;
    }

    // strip hvsc path from the file name
    string hvscFileName = m_fileName;
    size_t index = hvscFileName.find(m_hvscRoot);
    if (index != string::npos)
    {
	hvscFileName.erase(0, index + m_hvscRoot.length());
    }

    string str;
    if (!m_stil->hasCriticalError() && m_useGlobalComment)
    {
	char* globalComment = m_stil->getGlobalComment(hvscFileName.c_str());
	if (globalComment != NULL)
	{
	    str += globalComment;
	}
    }
    if (!m_stil->hasCriticalError())
    {
	char* stilEntry = m_stil->getEntry(hvscFileName.c_str(), 0);
	if (stilEntry != NULL)
	{
	    str += stilEntry;
	}
    }
    if (!m_stil->hasCriticalError())
    {
	char* bugEntry = m_stil->getBug(hvscFileName.c_str(), 0);
	if (bugEntry != NULL)
	{
	    str += bugEntry;
	}
    }
    if (m_stil->hasCriticalError())
    {
	m_statusString = m_stil->getErrorStr();
	return false;
    }

    // convert the stil text and remove all double whitespace characters
    unsigned int n = str.length();
    m_stilText.reserve(n);

    // start the scroll text with some space characters (to separate end
    // from beginning and to make sure the color effect has reached the end
    // of the line before the first character is visible)
    for (unsigned int i = 0; i < (STIL_EOT_SPACES-1); ++i)
    {
	m_stilText += Screen::iso2scr(' ');
    }

    bool space = true;
    bool realText = false;
    for (unsigned int i = 0; i < n; ++i)
    {
	if (isspace(str[i]))
	{
	    space = true;
	}
	else
	{
	    if (space) {
	       m_stilText += Screen::iso2scr(' ');
	       space = false;
	    }
	    m_stilText += Screen::iso2scr(str[i]);
	    realText = true;
	}
    }

    // check if the message contained at least one graphical character
    if (realText)
    {
	// end-of-text marker
	m_stilText += 0xff;
    }
    else
    {
	// no STIL text at all
	m_stilText.clear();
    }

    return true;
}


uint_least8_t
Psid64::findStilSpace(bool* pages, uint_least8_t scr,
                      uint_least8_t chars, uint_least8_t driver,
		      uint_least8_t size) const
{
    uint_least8_t firstPage = 0;
    for (unsigned int i = 0; i < MAX_PAGES; ++i)
    {
	if (pages[i] || ((scr && (scr <= i) && (i < (scr + NUM_SCREEN_PAGES))))
	    || ((chars && (chars <= i) && (i < (chars + NUM_CHAR_PAGES))))
	    || ((driver <= i) && (i < (driver + NUM_EXTDRV_PAGES))))
	{
	    if ((i - firstPage) >= size)
	    {
		return firstPage;
	    }
	    firstPage = i + 1;
	}
    }

    return 0;
}


uint_least8_t
Psid64::findDriverSpace(bool* pages, uint_least8_t scr,
                        uint_least8_t chars, uint_least8_t size) const
{
    uint_least8_t firstPage = 0;
    for (unsigned int i = 0; i < MAX_PAGES; ++i)
    {
	if (pages[i] || (scr && (scr <= i) && (i < (scr + NUM_SCREEN_PAGES)))
	    || (chars && (chars <= i) && (i < (chars + NUM_CHAR_PAGES))))
	{
	    if ((i - firstPage) >= size)
	    {
		return firstPage;
	    }
	    firstPage = i + 1;
	}
    }

    return 0;
}


void
Psid64::findFreeSpace()
/*--------------------------------------------------------------------------*
   In          : -
   Out         : m_driverPage		startpage of driver, 0 means no driver
		 m_screenPage		startpage of screen, 0 means no screen
		 m_charPage		startpage of chars, 0 means no chars
		 m_stilPage		startpage of stil, 0 means no stil
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
    bool pages[MAX_PAGES];
    unsigned int startp = m_tuneInfo.relocStartPage;
    unsigned int maxp = m_tuneInfo.relocPages;
    unsigned int endp;
    unsigned int i;
    unsigned int j;
    unsigned int k;
    uint_least8_t bank;
    uint_least8_t scr;
    uint_least8_t chars;
    uint_least8_t driver;

    // calculate size of the STIL text in pages
    uint_least8_t stilSize = (m_stilText.length() + 255) >> 8;

    m_screenPage = (uint_least8_t) (0x00);
    m_driverPage = (uint_least8_t) (0x00);
    m_charPage = (uint_least8_t) (0x00);
    m_stilPage = (uint_least8_t) (0x00);

    if (startp == 0x00)
    {
	// Used memory ranges.
	unsigned int used[] = {
	    0x00, 0x03,
	    0xa0, 0xbf,
	    0xd0, 0xff,
	    0x00, 0x00		// calculated below
	};

	// Finish initialization by setting start and end pages.
	used[6] = m_tuneInfo.loadAddr >> 8;
	used[7] = (m_tuneInfo.loadAddr + m_tuneInfo.c64dataLen - 1) >> 8;

	// Mark used pages in table.
	for (i = 0; i < MAX_PAGES; ++i)
	{
	    pages[i] = false;
	}
	for (i = 0; i < sizeof(used) / sizeof(*used); i += 2)
	{
	    for (j = used[i]; j <= used[i + 1]; ++j)
	    {
		pages[j] = true;
	    }
	}
    }
    else if ((startp != 0xff) && (maxp != 0))
    {
	// the available pages have been specified in the PSID file
	endp = min((startp + maxp), MAX_PAGES);

	// check that the relocation information does not use the following
	// memory areas: 0x0000-0x03FF, 0xA000-0xBFFF and 0xD000-0xFFFF
	if ((startp < 0x04)
	    || ((0xa0 <= startp) && (startp <= 0xbf))
	    || (startp >= 0xd0)
	    || ((endp - 1) < 0x04)
	    || ((0xa0 <= (endp - 1)) && ((endp - 1) <= 0xbf))
	    || ((endp - 1) >= 0xd0))
	{
	    return;
	}

	for (i = 0; i < MAX_PAGES; ++i)
	{
	    pages[i] = ((startp <= i) && (i < endp)) ? false : true;
	}
    }
    else
    {
	// not a single page is available
	return;
    }

    driver = 0;
    for (i = 0; i < 4; ++i)
    {
	// Calculate the VIC bank offset. Screens located inside banks 1 and 3
	// require a copy the character rom in ram. The code below uses a
	// little trick to swap bank 1 and 2 so that bank 0 and 2 are checked
	// before 1 and 3.
	bank = (((i & 1) ^ (i >> 1)) ? i ^ 3 : i) << 6;

	for (j = 0; j < 0x40; j += 4)
	{
	    // screen may not reside within the char rom mirror areas
	    if (!(bank & 0x40) && (0x10 <= j) && (j < 0x20))
		continue;

	    // check if screen area is available
	    scr = bank + j;
	    if (pages[scr] || pages[scr + 1] || pages[scr + 2]
		|| pages[scr + 3])
		continue;

	    if (bank & 0x40)
	    {
		// The char rom needs to be copied to RAM so let's try to find
		// a suitable location.
		for (k = 0; k < 0x40; k += 8)
		{
		    // char rom area may not overlap with screen area
		    if (k == (j & 0x38))
			continue;

		    // check if character rom area is available
		    chars = bank + k;
		    if (pages[chars] || pages[chars + 1]
			|| pages[chars + 2] || pages[chars + 3]
			|| pages[chars + 4] || pages[chars + 5]
			|| pages[chars + 6] || pages[chars + 7])
			continue;

		    driver =
			findDriverSpace (pages, scr, chars, NUM_EXTDRV_PAGES);
		    if (driver)
		    {
			m_driverPage = driver;
			m_screenPage = scr;
			m_charPage = chars;
			if (stilSize)
			{
			    m_stilPage = findStilSpace(pages, scr, chars,
						       driver, stilSize);
			}
			return;
		    }
		}
	    }
	    else
	    {
		driver = findDriverSpace(pages, scr, 0, NUM_EXTDRV_PAGES);
		if (driver)
		{
		    m_driverPage = driver;
		    m_screenPage = scr;
		    if (stilSize)
		    {
			m_stilPage = findStilSpace(pages, scr, 0, driver,
						   stilSize);
		    }
		    return;
		}
	    }
	}
    }

    if (!driver)
    {
	driver = findDriverSpace(pages, 0, 0, NUM_MINDRV_PAGES);
	m_driverPage = driver;
    }
}


//-------------------------------------------------------------------------
// Temporary hack till real bank switching code added

//  Input: A 16-bit effective address
// Output: A default bank-select value for $01.
uint8_t Psid64::iomap(uint_least16_t addr)
{
    // Force Real C64 Compatibility
    if (m_tuneInfo.compatibility == SIDTUNE_COMPATIBILITY_R64)
	return 0;     // Special case, converted to 0x37 later

    if (addr == 0)
	return 0;     // Special case, converted to 0x37 later
    if (addr < 0xa000)
	return 0x37;  // Basic-ROM, Kernal-ROM, I/O
    if (addr  < 0xd000)
	return 0x36;  // Kernal-ROM, I/O
    if (addr >= 0xe000)
	return 0x35;  // I/O only
    return 0x34;  // RAM only
}


void
Psid64::initDriver(uint_least8_t** mem, uint_least8_t** ptr, int* n)
{
    static const uint_least8_t psid_driver[] = {
#include "psiddrv.h"
    };
    static const uint_least8_t psid_extdriver[] = {
#include "psidextdrv.h"
    };
    const uint_least8_t* driver;
    uint_least8_t* psid_mem;
    uint_least8_t* psid_reloc;
    int psid_size;
    uint_least16_t reloc_addr;
    uint_least16_t addr;
    uint_least8_t vsa;	// video screen address
    uint_least8_t cba;	// character memory base address

    *ptr = NULL;
    *n = 0;

    // select driver
    if (m_screenPage == 0x00)
    {
	psid_size = sizeof(psid_driver);
	driver = psid_driver;
    }
    else
    {
	psid_size = sizeof(psid_extdriver);
	driver = psid_extdriver;
    }

    // Relocation of C64 PSID driver code.
    psid_mem = psid_reloc = new uint_least8_t[psid_size];
    if (psid_mem == NULL)
    {
	return;
    }
    memcpy(psid_reloc, driver, psid_size);
    reloc_addr = m_driverPage << 8;

    // undefined references in the drive code need to be added to globals
    globals_t globals;
    int screen = m_screenPage << 8;
    globals["screen"] = screen;
    int screen_songnum = 0;
    if (m_tuneInfo.songs > 1)
    {
	screen_songnum = screen + (10*40) + 24;
	if (m_tuneInfo.songs >= 100) ++screen_songnum;
	if (m_tuneInfo.songs >= 10) ++screen_songnum;
    }
    globals["screen_songnum"] = screen_songnum;
    globals["dd00"] = ((((m_screenPage & 0xc0) >> 6) ^ 3) | 0x04);
    vsa = (uint_least8_t) ((m_screenPage & 0x3c) << 2);
    cba = (uint_least8_t) (m_charPage ? (m_charPage >> 2) & 0x0e : 0x06);
    globals["d018"] = vsa | cba;

    if (!reloc65 ((char **) &psid_reloc, &psid_size, reloc_addr, &globals))
    {
	cerr << PACKAGE << ": Relocation error." << endl;
	return;
    }

    // Skip JMP table
    addr = 6;

    // Store parameters for PSID player.
    psid_reloc[addr++] = (uint_least8_t) (m_tuneInfo.initAddr ? 0x4c : 0x60);
    psid_reloc[addr++] = (uint_least8_t) (m_tuneInfo.initAddr & 0xff);
    psid_reloc[addr++] = (uint_least8_t) (m_tuneInfo.initAddr >> 8);
    psid_reloc[addr++] = (uint_least8_t) (m_tuneInfo.playAddr ? 0x4c : 0x60);
    psid_reloc[addr++] = (uint_least8_t) (m_tuneInfo.playAddr & 0xff);
    psid_reloc[addr++] = (uint_least8_t) (m_tuneInfo.playAddr >> 8);
    psid_reloc[addr++] = (uint_least8_t) (m_tuneInfo.songs);

    // get the speed bits (the driver only has space for the first 32 songs)
    uint_least32_t speed = 0;
    unsigned int songs = min(m_tuneInfo.songs, 32);
    for (unsigned int i = 0; i < songs; ++i)
    {
	if (m_tune[i + 1].songSpeed != SIDTUNE_SPEED_VBI)
	{
	    speed |= (1 << i);
	}
    }
    psid_reloc[addr++] = (uint_least8_t) (speed & 0xff);
    psid_reloc[addr++] = (uint_least8_t) ((speed >> 8) & 0xff);
    psid_reloc[addr++] = (uint_least8_t) ((speed >> 16) & 0xff);
    psid_reloc[addr++] = (uint_least8_t) (speed >> 24);

    psid_reloc[addr++] = (uint_least8_t) ((m_tuneInfo.loadAddr < 0x31a) ? 0xff : 0x05);
    psid_reloc[addr++] = iomap (m_tuneInfo.initAddr);
    psid_reloc[addr++] = iomap (m_tuneInfo.playAddr);

    if (m_screenPage != 0x00)
    {
	psid_reloc[addr++] = m_stilPage;
    }

    *mem = psid_mem;
    *ptr = psid_reloc;
    *n = psid_size;
}


void
Psid64::addFlag(bool &hasFlags, string flagName)
{
    if (hasFlags)
    {
	m_screen->write(", ");
    }
    else
    {
	hasFlags = true;
    }
    m_screen->write(flagName);
}


string
Psid64::toHexWord(uint16_t value) const
{
    ostringstream oss;

    oss << hex << uppercase << setfill('0') << setw(4) <<  value;

    return string(oss.str());
}


string
Psid64::toNumStr(int value) const
{
    ostringstream oss;

    oss << dec << value;

    return string(oss.str());
}


void
Psid64::drawScreen()
{
    m_screen->clear();

    // set title
    m_screen->move(5,1);
    m_screen->write("PSID64 v" VERSION " by Roland Hermans!");

    // characters for color line effect
    m_screen->poke( 4, 0, 0x70);
    m_screen->poke(35, 0, 0x6e);
    m_screen->poke( 4, 1, 0x5d);
    m_screen->poke(35, 1, 0x5d);
    m_screen->poke( 4, 2, 0x6d);
    m_screen->poke(35, 2, 0x7d);
    for (unsigned int i = 0; i < 30; ++i)
    {
	m_screen->poke(5 + i, 0, 0x40);
	m_screen->poke(5 + i, 2, 0x40);
    }

    // information lines
    m_screen->move(0, 4);
    m_screen->write("Name   : ");
    m_screen->write(string(m_tuneInfo.infoString[0]).substr(0,31));

    m_screen->write("\nAuthor : ");
    m_screen->write(string(m_tuneInfo.infoString[1]).substr(0,31));

    m_screen->write("\nRelease: ");
    m_screen->write(string(m_tuneInfo.infoString[2]).substr(0,31));

    m_screen->write("\nLoad   : $");
    m_screen->write(toHexWord(m_tuneInfo.loadAddr));
    m_screen->write("-$");
    m_screen->write(toHexWord(m_tuneInfo.loadAddr + m_tuneInfo.c64dataLen));

    m_screen->write("\nInit   : $");
    m_screen->write(toHexWord(m_tuneInfo.initAddr));

    m_screen->write("\nPlay   : ");
    if (m_tuneInfo.playAddr)
    {
	m_screen->write("$");
	m_screen->write(toHexWord(m_tuneInfo.playAddr));
    }
    else
    {
	m_screen->write("N/A");
    }

    m_screen->write("\nTunes  : ");
    m_screen->write(toNumStr(m_tuneInfo.songs));
    if (m_tuneInfo.songs > 1)
    {
	m_screen->write(" (now playing");
    }

    bool hasFlags = false;
    m_screen->write("\nFlags  : ");
    if (m_tuneInfo.compatibility == SIDTUNE_COMPATIBILITY_PSID)
    {
	addFlag(hasFlags, "PlaySID");
    }
    switch (m_tuneInfo.clockSpeed)
    {
    case SIDTUNE_CLOCK_PAL:
	addFlag(hasFlags, "PAL");
	break;
    case SIDTUNE_CLOCK_NTSC:
	addFlag(hasFlags, "NTSC");
	break;
    case SIDTUNE_CLOCK_ANY:
	addFlag(hasFlags, "PAL/NTSC");
        break;
    default:
        break;
    }
    switch (m_tuneInfo.sidModel)
    {
    case SIDTUNE_SIDMODEL_6581:
	addFlag(hasFlags, "6581");
	break;
    case SIDTUNE_SIDMODEL_8580:
	addFlag(hasFlags, "8580");
	break;
    case SIDTUNE_SIDMODEL_ANY:
	addFlag(hasFlags, "6581/8580");
	break;
    default:
        break;
    }
    if (!hasFlags)
    {
	m_screen->write("-");
    }
    m_screen->write("\nClock  :   :  :");

    // some additional text
    m_screen->write("\n\n\
This is an experimental PSID player that\n\
supports the PSID V2 NG standard. The\n\
driver and screen are relocated based on\n\
information stored inside the PSID.");

    // flashing bottom line (should be exactly 38 characters)
    m_screen->move(1,24);
    m_screen->write("Website: http://psid64.sourceforge.net");
}
