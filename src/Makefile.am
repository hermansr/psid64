# Process this file with automake to produce Makefile.in

SUBDIRS = libpsid64 sidtune sidutils

# enable warnings and turn them into errors to enforce warning free code
AM_CFLAGS = -Wall -Werror

bin_PROGRAMS = psid64

psid64_CFLAGS = $(AM_CFLAGS)

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
