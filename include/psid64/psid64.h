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

#ifndef PSID64_H
#define PSID64_H

#include <iostream>
#include <string>

#include <sidplay/utils/SidDatabase.h>
#include <sidplay/utils/SidTuneMod.h>


//////////////////////////////////////////////////////////////////////////////
//                   F O R W A R D   D E C L A R A T O R S
//////////////////////////////////////////////////////////////////////////////

class Screen;
class STIL;


//////////////////////////////////////////////////////////////////////////////
//                           D E F I N I T I O N S
//////////////////////////////////////////////////////////////////////////////

typedef struct block_s
{
    uint_least16_t load;
    uint_least16_t size;
    const uint_least8_t* data;
    std::string description;
}
block_t;

class Psid64
{
public:
    Psid64();
    ~Psid64();

    bool setHvscRoot(std::string hvscRoot);

    inline const std::string getHvscRoot() const
    {
        return m_hvscRoot;
    }

    bool setDatabaseFileName(std::string databaseFileName);

    inline const std::string getDatabaseFileName() const
    {
        return m_databaseFileName;
    }

    inline void setBlankScreen(bool blankScreen)
    {
        m_blankScreen = blankScreen;
    }

    inline bool getBlankScreen() const
    {
        return m_blankScreen;
    }

    inline void setUseGlobalComment(bool useGlobalComment)
    {
        m_useGlobalComment = useGlobalComment;
    }

    inline bool getUseGlobalComment() const
    {
        return m_useGlobalComment;
    }

    inline void setVerbose(bool verbose)
    {
        m_verbose = verbose;
    }

    inline bool getVerbose() const
    {
        return m_verbose;
    }

    inline const char* getStatus() const
    {
	return m_statusString;
    }

    bool load(const char* fileName);
    bool convert();
    bool save(const char* fileName);
    bool write(std::ostream& out = std::cout);

private:
    static const unsigned int MAX_BLOCKS = 4;
    static const unsigned int MAX_PAGES = 256; // number of memory pages
    static const unsigned int NUM_MINDRV_PAGES = 2; // driver without screen display
    static const unsigned int NUM_EXTDRV_PAGES = 4; // driver with screen display
    static const unsigned int NUM_SCREEN_PAGES = 4; // size of screen in pages
    static const unsigned int NUM_CHAR_PAGES = 8; // size of charset in pages
    static const unsigned int STIL_EOT_SPACES = 5; // number of spaces before EOT

    // error and status message strings
    static const char* txt_relocOverlapsImage;
    static const char* txt_notEnoughC64Memory;
    static const char* txt_fileIoError;
    static const char* txt_noSidTuneLoaded;
    static const char* txt_noSidTuneConverted;

    // configuration options
    bool m_blankScreen;
    bool m_useGlobalComment;
    bool m_verbose;
    std::string m_hvscRoot;
    std::string m_databaseFileName;
//    std::string m_outputFileName;
//    std::ostream m_logFile;

    // state data
    bool m_status;
    const char* m_statusString;   // error/status message of last operation

    // other internal data
    std::string m_fileName;
    SidTuneMod m_tune;
    SidTuneInfo m_tuneInfo;
    SidDatabase m_database;
    STIL *m_stil;

    // conversion data
    Screen *m_screen;
    std::string m_stilText;
    uint_least8_t m_driverPage; // startpage of driver, 0 means no driver
    uint_least8_t m_screenPage; // startpage of screen, 0 means no screen
    uint_least8_t m_charPage; // startpage of chars, 0 means no chars
    uint_least8_t m_stilPage; // startpage of stil, 0 means no stil

    // converted file
    uint_least8_t *m_programData;
    unsigned int m_programSize;

    // member functions
    bool Psid64::formatStilText();
    uint_least8_t Psid64::findStilSpace(bool* pages, uint_least8_t scr,
					uint_least8_t chars,
					uint_least8_t driver,
					uint_least8_t size) const;
    uint_least8_t Psid64::findDriverSpace(bool* pages, uint_least8_t scr,
				          uint_least8_t chars,
					  uint_least8_t size) const;
    void Psid64::findFreeSpace();
    void Psid64::initDriver(uint_least8_t** ptr, int* n);
    void Psid64::addFlag(bool &hasFlags, std::string flagName);
    std::string Psid64::toHexWord(uint_least16_t value) const;
    std::string Psid64::toNumStr(int value) const;
    void Psid64::drawScreen();
//    int Psid64::block_cmp(block_t* a, block_t* b);
//    char* Psid64::build_output_filename(char* p_psid_file);
//    std::string Psid64::build_output_filename(const std::string &p_psid_file);
};


inline std::ostream& operator << (std::ostream& out, Psid64 &psid64)
{
    psid64.write(out);
    return out;
}

#endif // PSID64_H
