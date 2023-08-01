/*
    $Id$

    psid64 - create a C64 executable from a PSID file
    Copyright (C) 2001-2014  Roland Hermans <rolandh@users.sourceforge.net>

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

#include "ConsoleApp.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <dirent.h>
#include <errno.h>

#include <algorithm>
#include <cstdlib>
#include <map>
#include <sstream>
#include <vector>

using std::cerr;
using std::cout;
using std::endl;
using std::istringstream;
using std::map;
using std::sort;
using std::string;
using std::vector;

#define STR_GETOPT_OPTIONS		":bcghi:no:p:r:s:t:vV"
#define ACCEPTED_PATH_SEPARATORS	"/\\"

#ifdef _WIN32
#define PATH_SEPARATOR			"\\"
#else
#define PATH_SEPARATOR			"/"
#endif

#ifndef HAVE_LSTAT
#define lstat(path, buf)		stat(path, buf)
#endif

#ifndef ACCESSPERMS
#define ACCESSPERMS			(S_IRWXU|S_IRWXG|S_IRWXO)
#endif

#ifdef HAVE_MKDIR
#ifdef MKDIR_TAKES_ONE_ARG
#define mkdir(path, mode)		mkdir(path)
#endif
#else
#ifdef HAVE__MKDIR
#define mkdir(path, mode)		_mkdir(path)
#else
#error "Don't know how to create a directory on this system."
#endif
#endif

typedef map<string,Psid64::Theme> ThemesMap;


// constructor

ConsoleApp::ConsoleApp() :
    m_sidPostfix(".sid"),
    m_prgPostfix(".prg"),
    m_verbose(false),
    m_outputPathName()
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
    cout << "  -i, --initial-song=NUM override the initial song to play" << endl;
    cout << "  -n, --no-driver        convert SID to C64 program file without driver code" << endl;
    cout << "  -o, --output=PATH      specify output file or directory" << endl;
    cout << "  -p, --player-id=FILE   specify SID ID config file for player identification" << endl;
    cout << "  -r, --root=PATH        specify HVSC root directory" << endl;
    cout << "  -s, --songlengths=FILE specify HVSC song length database" << endl;
    cout << "  -t, --theme=THEME      specify a visual theme for the driver" << endl;
    cout << "                         use `help' to show the list of available themes" << endl;
    cout << "  -v, --verbose          explain what is being done" << endl;
    cout << "  -h, --help             display this help and exit" << endl;
    cout << "  -V, --version          output version information and exit" << endl;
#else
    cout << "  -b                     use a minimal driver that blanks the screen" << endl;
    cout << "  -c                     compress output file with Exomizer" << endl;
    cout << "  -g                     include the global comment STIL text" << endl;
    cout << "  -i                     override the initial song to play" << endl;
    cout << "  -n                     convert SID to C64 program file without driver code" << endl;
    cout << "  -o                     specify output file or directory" << endl;
    cout << "  -p                     specify SID ID config file for player identification" << endl;
    cout << "  -r                     specify HVSC root directory" << endl;
    cout << "  -s                     specify HVSC song length database" << endl;
    cout << "  -t                     specify a visual theme for the driver" << endl;
    cout << "                         use `help' to show the list of available themes" << endl;
    cout << "  -v                     explain what is being done" << endl;
    cout << "  -h                     display this help and exit" << endl;
    cout << "  -V                     output version information and exit" << endl;
#endif
    cout << endl << "Report bugs to <rolandh@users.sourceforge.net>." << endl;
}


bool
ConsoleApp::isdir(const string& path)
{
    struct stat s;
    return (lstat(path.c_str(), &s) == 0) && S_ISDIR(s.st_mode);
}


string
ConsoleApp::basename(const string& path)
{
    string filename(path);

    const size_t index = filename.find_last_of(ACCEPTED_PATH_SEPARATORS);
    if (index != std::string::npos)
    {
	filename.erase(0, index + 1);
    }

    return filename;
}


string
ConsoleApp::buildOutputFileName(const string& sidFileName, const string& outputPathName) const
{
    string prgFileName;
    bool replaceSuffix = false;

    if (outputPathName.empty())
    {
	prgFileName = sidFileName;
	replaceSuffix = true;
    }
    else
    {
	// use filename or directory, e.g. specified by --output option
	prgFileName = outputPathName;
	if (isdir(prgFileName))
	{
	    prgFileName += PATH_SEPARATOR + basename(sidFileName);
	    replaceSuffix = true;
	}
    }

    if (replaceSuffix)
    {
	// replace the .sid extension by .prg extension
	int index = prgFileName.length() - m_sidPostfix.length();
	if ((index >= 0) && (prgFileName.substr(index) == m_sidPostfix))
	{
	    prgFileName.erase(index);
	}
	prgFileName += m_prgPostfix;
    }

    return prgFileName;
}


bool ConsoleApp::convertFile(const string& inputFileName, const string& outputFileName)
{
    // read the PSID file
    if (m_verbose)
    {
	cerr << "Reading file `" << inputFileName << "'" << endl;
    }
    if (!m_psid64.load(inputFileName.c_str()))
    {
	cerr << "Error loading '" << inputFileName << "': "
	     << m_psid64.getStatus() << endl;
	return false;
    }

    // convert the PSID file
    if (!m_psid64.convert())
    {
	cerr << "Error converting '" << inputFileName << "': "
	     << m_psid64.getStatus() << endl;
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
	    cerr << "Error writing to standard output: "
	         << m_psid64.getStatus() << endl;
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
	    cerr << "Error writing '" << outputFileName << "': "
	         << m_psid64.getStatus() << endl;
	    return false;
	}
    }

    return true;
}


bool ConsoleApp::convertDir(const string& inputDirName, const string& outputDirName)
{
    bool retval = true;
    const bool recursive = true;

    if (!isdir(outputDirName))
    {
	if (mkdir(outputDirName.c_str(), ACCESSPERMS) != 0)
	{
	    cerr << PACKAGE << ": Cannot create directory `" << outputDirName << "': " << strerror(errno) << "\n";
	    retval = false;
	}
    }

    DIR *dp;
    if ((dp = opendir(inputDirName.c_str())) == NULL)
    {
	cerr << PACKAGE << ": Cannot access `" << inputDirName << "': " << strerror(errno) << "\n";
	retval = false;
    }
    else
    {
	vector<string> dirs;
	vector<string> files;
	struct dirent *dirp;
	while ((dirp = readdir(dp)) != NULL)
	{
            if ((strcmp(dirp->d_name, ".") != 0)
	        && (strcmp(dirp->d_name, "..") != 0))
	    {
                string path = inputDirName + PATH_SEPARATOR + dirp->d_name;
                if (isdir(path))
		{
        	    if (recursive)
        	    {
                	dirs.push_back(dirp->d_name);
                    }
                }
		else
		{
		    // ignore files with an unknown extension
		    int index = path.length() - m_sidPostfix.length();
		    if ((index >= 0) && (path.substr(index) == m_sidPostfix))
		    {
			files.push_back(dirp->d_name);
		    }
                }
            }
        }
        closedir(dp);

	// process the subdirectories
        sort(dirs.begin(), dirs.end());
	for (vector<string>::const_iterator it = dirs.begin();
	     (it != dirs.end()) && retval; ++it)
        {
	    string newInputDirName = inputDirName + PATH_SEPARATOR + *it;
	    string newOutputDirName = outputDirName + PATH_SEPARATOR + *it;
            retval = retval && convertDir(newInputDirName, newOutputDirName);
        }

	// process files
        sort(files.begin(), files.end());
	for (vector<string>::const_iterator it = files.begin();
	     (it != files.end()) && retval; ++it)
        {
	    string inputFileName = inputDirName + PATH_SEPARATOR + *it;
 	    string outputFileName = buildOutputFileName(*it, outputDirName);
            retval = retval && convertFile(inputFileName, outputFileName);
        }
    }

    return retval;
}


bool ConsoleApp::convert(const string& inputPathName)
{
    if (isdir(inputPathName))
    {
	string outputDirName;
	if (m_outputPathName.empty())
	{
	    outputDirName = inputPathName;
	}
	else
	{
	    outputDirName = m_outputPathName;
	    if (isdir(outputDirName))
	    {
		outputDirName += PATH_SEPARATOR + basename(inputPathName);
	    }
	}
	return convertDir(inputPathName, outputDirName);
    }
    else
    {
	string outputFileName = buildOutputFileName(inputPathName, m_outputPathName);
	return convertFile(inputPathName, outputFileName);
    }
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
	{"initial-song", 1, NULL, 'i'},
        {"no-driver", 0, NULL, 'n'},
	{"output", 1, NULL, 'o'},
	{"player-id", 1, NULL, 'p'},
	{"root", 1, NULL, 'r'},
	{"songlengths", 1, NULL, 's'},
	{"theme", 1, NULL, 't'},
	{"verbose", 0, NULL, 'v'},
	{"version", 0, NULL, 'V'},
	{NULL, 0, NULL, 0}
    };
#endif
    ThemesMap themes;
    themes["blue"] = Psid64::THEME_BLUE;
    themes["c1541_ultimate"] = Psid64::THEME_C1541_ULTIMATE;
    themes["coal"] = Psid64::THEME_COAL;
    themes["default"] = Psid64::THEME_DEFAULT;
    themes["dutch"] = Psid64::THEME_DUTCH;
    themes["kernal"] = Psid64::THEME_KERNAL;
    themes["light"] = Psid64::THEME_LIGHT;
    themes["mondriaan"] = Psid64::THEME_MONDRIAAN;
    themes["ocean"] = Psid64::THEME_OCEAN;
    themes["pencil"] = Psid64::THEME_PENCIL;
    themes["rainbow"] = Psid64::THEME_RAINBOW;
    string hvscRoot;
    string databaseFileName;
    string sidIdConfigFileName;

    // set default configuration
    m_psid64.setVerbose(false);
    m_psid64.setUseGlobalComment(false);
    m_psid64.setBlankScreen(false);
    m_psid64.setNoDriver(false);
    const char* hvscBase = getenv ("HVSC_BASE");
    if (hvscBase != NULL)
    {
	hvscRoot = hvscBase;
    }
    const char* hvscSongLengths = getenv ("HVSC_SONGLENGTHS");
    if (hvscSongLengths != NULL)
    {
	databaseFileName = hvscSongLengths;
    }
    const char* sididcfg = getenv ("SIDIDCFG");
    if (sididcfg != NULL)
    {
	sidIdConfigFileName = sididcfg;
    }
    m_outputPathName.clear();

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
	case 'i':
	    {
		istringstream istr(optarg);
		int initialSong;
		istr >> initialSong;
		if ((1 <= initialSong) && (initialSong <= 255))
		{
		    m_psid64.setInitialSong(initialSong);
		}
		else
		{
		    cerr << "initial song should be an integer number between 1 and 255" << endl;
		}
	    }
	    break;
	case 'n':
	    m_psid64.setNoDriver(true);
	    break;
	case 'o':
	    m_outputPathName = optarg;
	    break;
	case 'p':
	    sidIdConfigFileName = optarg;
	    break;
	case 'r':
	    hvscRoot = optarg;
	    break;
	case 's':
	    databaseFileName = optarg;
	    break;
	case 't':
	    {
	        if (strcmp(optarg, "help") == 0)
		{
		    for (ThemesMap::const_iterator it = themes.begin();
		         it != themes.end(); ++it)
		    {
			cout << it->first << "\n";
		    }
		    return true;
		}
		else
		{
		    ThemesMap::const_iterator it = themes.find(optarg);
		    if (it != themes.end())
		    {
			m_psid64.setTheme(it->second);
		    }
		    else
		    {
			cerr << PACKAGE << ": unknown theme `" << optarg
		             << "'" << endl;
			++errflg;
		    }
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

    // check that output is an existing directory when having multiple inputs
    if (((argc - optind) > 1) && (!m_outputPathName.empty()) && (!isdir(m_outputPathName)))
    {
	cerr << PACKAGE << ": target `" << m_outputPathName << "' is not a directory" << endl;
	return false;
    }

    if (!hvscRoot.empty())
    {
	if (!m_psid64.setHvscRoot(hvscRoot))
	{
	    cerr << m_psid64.getStatus() << ": STILView will be disabled" << endl;
	}

	if (databaseFileName.empty())
	{
	    // use song length database from HVSC if no database was specified
	    databaseFileName = hvscRoot + PATH_SEPARATOR + "DOCUMENTS"
	                       + PATH_SEPARATOR + "Songlengths.txt";
	}
    }

    if (!databaseFileName.empty())
    {
	if (!m_psid64.setDatabaseFileName(databaseFileName))
	{
 	    cerr << m_psid64.getStatus() << ": song lengths will be disabled" << endl;
 	}
    }

    if (!sidIdConfigFileName.empty())
    {
	if (!m_psid64.setSidIdConfigFileName(sidIdConfigFileName))
	{
	    cerr << m_psid64.getStatus() << ": player identification will be disabled" << endl;
	}
    }

    while (optind < argc)
    {
	string inputPathName = argv[optind++];

	// remove any trailing path separators
	size_t index = inputPathName.find_last_not_of(ACCEPTED_PATH_SEPARATORS);
	if (index != string::npos)
	{
	    inputPathName.erase(index + 1);
	}

	if (!convert(inputPathName))
	{
	    return false;
	}
    }

    return true;
}
