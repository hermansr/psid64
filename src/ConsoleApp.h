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

#ifndef CONSOLEAPP_H
#define CONSOLEAPP_H

#include <string>

#include <psid64/psid64.h>

class ConsoleApp
{
public:
    ConsoleApp();
    ~ConsoleApp();

    bool main(int argc, char **argv);

private:
    const std::string m_sidPostfix;
    const std::string m_prgPostfix;

    bool m_verbose;
    std::string m_outputPathName;
    bool m_outputPathIsDir;

    Psid64 m_psid64;

    static void printUsage ();
    static void printHelp ();

    static bool isdir(const std::string& path);
    static std::string basename(const std::string& path);
    std::string buildOutputFileName(const std::string& sidFileName) const;
    bool convert(const std::string inputFileName);
};

#endif // CONSOLEAPP_H
