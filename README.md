PSID64 README
=============

Introduction
------------

PSID64 is a program that automatically generates a C64 self extracting
executable from a PSID file. The executable contains the PSID data, a
pre-relocated player and may also - if there is enough space available in the
C64 environment - contain a demonstration program with information about the
PSID file.

PSID files contain music code and data originally generated on the Commodore 64
(C64). If you're new to PSID files please refer to the introduction chapter of
the High Voltage SID Collection documentation for a brief introduction into SID
music.

Before PSID64, playing PSID files on a C64 or a C64 emulator has been a rather
frustrating experience. The fundamental problem of where to locate the PSID
driver has simply not been appropriately addressed. Actually, having a PSID
file play at all has been pure luck, since there has been made no attempt to
find a memory range that is not written to by the music code. Not surprisingly,
a large number of PSID files could not be played at all using existing C64
computers and C64 emulator SID players.

The document "Proposal for C64 compatible PSID files", written by Dag Lem and
Simon White extends the PSID format with relocation information. PSID64 is the
first implementation of a PSID player intended for playback on a real C64 or
C64 environment that uses this relocation information stored in the PSID
header.


Usage
-----

PSID64 is invoked from the command line as follows:

    psid64 [OPTION]... PSID_FILE...

where PSID_FILE is the names of one or more the PSID files or directories.

If a name designates a file, the resulting C64 executable is written to the
current directory, or to the file or directory specified by the -o option.

If a name designates a directory, PSID64 processes all the .sid files in that
directory and all of its subdirectories. Without using the -o option, each
resulting C64 executable is written to the same location as its source .sid
file. With the -o option, the basename of the PSID_FILE name is created in the
specified output directory. If the PSID_FILE name ends in a slash or backslash,
or if the specified output directory does not yet exist, this changes the
behavior and the basename directory is not created. The subdirectories of
PSID_FILE that contain .sid files are created in the output location.

Options available:

    -b, --blank-screen     use a minimal driver that blanks the screen
    -c, --compress         compress output file with Exomizer
    -g, --global-comment   include the global comment STIL text
    -i, --initial-song=NUM override the initial song to play
    -n, --no-driver        convert SID to C64 program file without driver code
    -o, --output=PATH      specify output file or directory
    -p, --player-id=FILE   specify SID ID config file for player identification
    -r, --root=PATH        specify HVSC root directory
    -s, --songlengths=FILE specify HVSC song length database
    -t, --theme=THEME      specify a visual theme for the driver
                           use `help' to show the list of available themes
    -v, --verbose          explain what is being done
    -h, --help             display this help and exit
    -V, --version          output version information and exit

The HVSC path recognition is a bit rudimentary. For this to work properly it is
required to include the HVSC path in the filename of the files or directories
to be converted. If the path strings don't match, the STIL info will not be
included in the generated .prg files.


Example usage
-------------

Convert a single file and write the output to the current directory:

    psid64 input.sid

Convert a single file and write the output to output.prg:

    psid64 -o output.prg input.sid

Convert multiple files, write all the outputs to the current directory:

    psid64 input1.sid a/input2.sid b/input3.sid

Convert all files in current directory and all subdirectories:

    psid64 .

On a UNIX-like system, convert the complete HVSC collection with STIL, song
length, and player ID information to the directory hvsc_as_prg:

    psid64 -v -c -p sidid.cfg -r ~/C64Music -o hvsc_as_prg ~/C64Music/

On a Windows-like system, convert the complete HVSC collection with STIL, song
length, and player ID information to the directory hvsc_as_prg:

    psid64 -v -c -p sidid.cfg -r C:\C64Music -o hvsc_as_prg C:\C64Music\


Environment variables
---------------------

    HVSC_BASE                Default HVSC root directory
    HVSC_SONGLENGTHS         Default HVSC song length database
    SIDIDCFG                 Default SID ID configuration file


Keyboard control
----------------

    1-0, A-Z                 Select song 1-36
    +                        Select next song (2)
    -                        Select previous song (2)
    INST/DEL                 Toggle screen blanking on/off (2)
    RUN/STOP                 Stop playback
    LEFT ARROW               Fast forward (1)
    SHIFT LEFT / LOCK        Show rastertime used by player (1)
    CONTROL + CBM + DEL      Reset computer (2)

(1) not available for custom players (i.e. play address is $0000).

(2) not available in minimal driver (i.e. the driver that blanks the screen)


Joystick control
----------------

Except when using the minimal driver, a joystick connected to port two can be
used to control some functions of the player.

    Up                       Select next song
    Down                     Select previous song
    Left                     Stop playback
    Right                    Restart current song
    Fire button              Fast forward (1)

(1) not available for custom players (i.e. play address is $0000).


Credits
-------

PSID64 contains the following contributed or derived work. In the order they
first supplied contributions or code was derived from their work:

    Dag Lem                  PSID driver reference implementation
    Simon White              SidUtils library
    Michael Schwendt         SidTune library
    LaLa                     STILView library
    Magnus Lind              Exomizer compressor

Credit where credit is due, so if I missed anyone please let me know.


Known problems and limitations
------------------------------

At least two free 256 byte pages are required for the minimal driver and at
least nine 256 byte pages are required for the extended driver.

The accuracy of the clock cannot be guaranteed while fast forwarding a song.
This is due to the fact that the driver does not know how often the play
function is called. PSID64 assumes that the play function called 50 times per
second on a PAL machine and 60 times per second on an NTSC machine.

The scroller might show some artifacts when interrupts occur just before or just
after the scroller area. To guarantee maximal compatibility with SID tunes it's
neither feasible nor desired to program a scroller based on code that generates
interrupts. Any ideas on how to improve the scroller code are welcome.


Other tools
-----------

Andre Fachat's relocating 6502 cross assembler XA is used to create the
relocatable driver code.

https://www.floodgap.com/retrotech/xa/

The DJGPP cross-compiler djcross-gcc-12.2.0 has been used to build the MS-DOS
executable.

https://www.delorie.com/djgpp/

https://www.delorie.com/pub/djgpp/rpms/

UPX is a free, portable, extendable, high-performance executable packer for
several different executable formats. It has been used to create the compressed
executable of the MS-DOS release.

https://upx.sourceforge.net/

The Fedora MinGW-w64 cross-compiler environment has been used to build the
Windows 32-bit and 64-bit executables.

https://fedoraproject.org/wiki/MinGW

https://www.mingw-w64.org/


Copyright and licensing
-----------------------

SPDX-License-Identifier: GPL-2.0-or-later

Copyright 2001-2023 Roland Hermans <rolandh@users.sourceforge.net>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.


Website
-------

https://www.psid64.org/ or https://psid64.sourceforge.io/
