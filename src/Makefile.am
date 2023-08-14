# SPDX-License-Identifier: GPL-2.0-or-later

# Process this file with automake to produce Makefile.in

SUBDIRS = libpsid64 sidtune sidutils

AM_CXXFLAGS = $(WARNINGCXXFLAGS)

bin_PROGRAMS = psid64

psid64_SOURCES = \
	ConsoleApp.cpp \
	ConsoleApp.h \
	main.cpp

psid64_LDADD = \
	libpsid64/libpsid64.a \
	sidtune/libsidtune.a \
	sidutils/libsidutils.a \
	sidutils/MD5/libMD5.a \
	sidutils/ini/libini.a
