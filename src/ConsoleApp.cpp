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

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "ConsoleApp.h"

using std::cerr;
using std::cout;
using std::endl;
using std::string;

#define STR_GETOPT_OPTIONS		":bcgho:r:s:vV"


// constructor

ConsoleApp::ConsoleApp() :
    m_sidPostfix(".sid"),
    m_prgPostfix(".prg"),
    m_verbose(false)
{
}

// destructor

ConsoleApp::~ConsoleApp()
{
}


void ConsoleApp::printUsage ()
{
    cout << "Usage: " << PACKAGE << " [OPTION]... PSID_FILE..." << endl;
}


void ConsoleApp::printHelp ()
{
    printUsage ();
    cout << endl;
#ifdef HAVE_GETOPT_LONG
    cout << "  -b, --blank-screen     use a minimal driver that blanks the screen" << endl;
    cout << "  -c, --compress         compress output file with Exomizer" << endl;
    cout << "  -g, --global-comment   include the global comment STIL text" << endl;
    cout << "  -o, --output=FILE      specify output file" << endl;
    cout << "  -r, --root             specify HVSC root directory" << endl;
    cout << "  -s, --songlengths=FILE specify HVSC song length database" << endl;
    cout << "  -v, --verbose          explain what is being done" << endl;
    cout << "  -h, --help             display this help and exit" << endl;
    cout << "  -V, --version          output version information and exit" << endl;
#else
    cout << "  -b                     use a minimal driver that blanks the screen" << endl;
    cout << "  -c                     compress output file with Exomizer" << endl;
    cout << "  -g                     include the global comment STIL text" << endl;
    cout << "  -o                     specify output file" << endl;
    cout << "  -r                     specify HVSC root directory" << endl;
    cout << "  -s                     specify HVSC song length database" << endl;
    cout << "  -v                     explain what is being done" << endl;
    cout << "  -h                     display this help and exit" << endl;
    cout << "  -V                     output version information and exit" << endl;
#endif
    cout << endl << "Report bugs to <rolandh@users.sourceforge.net>." << endl;
}


string
ConsoleApp::buildOutputFileName (const string& sidFileName) const
{
    string prgFileName;

    if (m_outputFileName.empty() == false)
    {
	// use filename specified by --output option
	prgFileName = m_outputFileName;
    }
    else
    {
	// replace the .sid extension by .prg extension
	prgFileName = sidFileName;
	unsigned int index = sidFileName.length() - m_sidPostfix.length();
	if (sidFileName.substr(index) == m_sidPostfix)
	{
	    prgFileName.erase(index);
	}
	prgFileName += m_prgPostfix;
    }

    return prgFileName;
}


bool ConsoleApp::convert(const string inputFileName)
{
    string outputFileName = buildOutputFileName (inputFileName);

    // read the PSID file
    if (m_verbose)
    {
	cerr << "Reading file `" << inputFileName << "'" << endl;
    }
    if (!m_psid64.load(inputFileName.c_str()))
    {
	cerr << m_psid64.getStatus() << endl;
	return false;
    }

    // convert the PSID file
    if (!m_psid64.convert())
    {
	cerr << m_psid64.getStatus() << endl;
	return false;
    }

    // write the C64 program file
    if (outputFileName == "-")
    {
	if (m_verbose)
	{
	    cerr << "Writing C64 executable to standard output" << endl;
	}
	if (!m_psid64.write())
	{
	    cerr << m_psid64.getStatus() << endl;
	    return false;
	}
    }
    else
    {
	if (m_verbose)
	{
	    cerr << "Writing C64 executable `" << outputFileName << "'" << endl;
	}
	if (!m_psid64.save(outputFileName.c_str()))
	{
	    cerr << m_psid64.getStatus() << endl;
	    return false;
	}
    }

    return true;
}


bool ConsoleApp::main(int argc, char **argv)
{
    int                     c;
    int                     errflg = 0;
#ifdef HAVE_GETOPT_LONG
    int                     option_index = 0;
    static struct option    long_options[] = {
	{"blank-screen", 0, NULL, 'b'},
	{"compress", 0, NULL, 'c'},
	{"global-comment", 0, NULL, 'g'},
	{"help", 0, NULL, 'h'},
	{"output", 1, NULL, 'o'},
	{"root", 1, NULL, 'r'},
	{"songlengths", 1, NULL, 's'},
	{"verbose", 0, NULL, 'v'},
	{"version", 0, NULL, 'V'},
	{NULL, 0, NULL, 0}
    };
#endif

    // set default configuration
    m_psid64.setVerbose(false);
    m_psid64.setUseGlobalComment(false);
    m_psid64.setBlankScreen(false);
    const char* hvscBase = getenv ("HVSC_BASE");
    if (hvscBase != NULL)
    {
	string hvscRoot(hvscBase);
	m_psid64.setHvscRoot(hvscRoot);
    }
    const char* hvscSongLengths = getenv ("HVSC_SONGLENGTHS");
    if (hvscSongLengths != NULL)
    {
	string databaseFileName(hvscSongLengths);
	m_psid64.setDatabaseFileName(databaseFileName);
    }
    m_outputFileName = "";

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
	case 'b':
	    m_psid64.setBlankScreen(true);
	    break;
	case 'c':
	    m_psid64.setCompress(true);
	    break;
	case 'g':
	    m_psid64.setUseGlobalComment(true);
	    break;
	case 'h':
	    printHelp ();
	    exit (0);
	    break;
	case 'o':
	    m_outputFileName = optarg;
	    break;
	case 'r':
	    {
		string hvscRoot(optarg);
		if (!m_psid64.setHvscRoot(hvscRoot))
		{
		    cerr << m_psid64.getStatus() << ": STILView will be disabled" << endl;
		}
	    }
	    break;
	case 's':
	    {
		string databaseFileName(optarg);
		if (!m_psid64.setDatabaseFileName(databaseFileName))
		{
 		    cerr << m_psid64.getStatus() << ": song lengths will be disabled" << endl;
 		}
	    }
	    break;
	case 'v':
	    m_verbose = true;
	    m_psid64.setVerbose(true);
	    break;
	case 'V':
	    cout << PACKAGE << " version " << VERSION << endl;
	    exit (0);
	    break;
	case ':':
	    cerr << PACKAGE << ": option requires an argument -- "
	         << (char) optopt << endl;
	    ++errflg;
	    break;
	case '?':
	    cerr << PACKAGE << ": invalid option -- " << (char) optopt << endl;
	    ++errflg;
	    break;
	default:
	    cerr << PACKAGE << ":" << __FILE__ << ":" << __LINE__
		 << ": software error" << endl;
	    return false;
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
	printUsage ();
	++errflg;
    }

    if (errflg)
    {
	cerr << "Try `" << PACKAGE << " "
#ifdef HAVE_GETOPT_LONG
	     << "--help"
#else
	     << "-h"
#endif
	     << "' for more information." << endl;
	return false;
    }

    while (optind < argc)
    {
	if (!convert(argv[optind++]))
	{
	    return false;
	}
    }

    return true;
}
