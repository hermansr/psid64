# SPDX-License-Identifier: GPL-2.0-or-later

# Process this file with automake to produce Makefile.in

SUBDIRS = src

pkginclude_HEADERS = \
	include/psid64/psid64.h

noinst_HEADERS = \
	include/sidplay/Buffer.h \
	include/sidplay/SidTune.h \
	include/sidplay/SmartPtr.h \
	include/sidplay/sidconfig.h \
	include/sidplay/sidconfig.h.in \
	include/sidplay/sidendian.h \
	include/sidplay/sidint.h \
	include/sidplay/sidtypes.h \
	include/sidplay/utils/libini.h \
	include/sidplay/utils/SidDatabase.h \
	include/sidplay/utils/SidTuneMod.h

DISTCLEANFILES = \
	include/sidplay/sidint.h

EXTRA_DIST = \
	Doxyfile \
	psid64.spec \
	macros/ax_create_stdint_h.m4 \
	macros/ax_func_mkdir.m4 \
	macros/sidtune.m4
# AUTHORS COPYING INSTALL NEWS.md README.md TODO

ACLOCAL_AMFLAGS = -I macros
