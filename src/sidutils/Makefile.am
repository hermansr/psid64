SUBDIRS = MD5 ini

noinst_LIBRARIES = libsidutils.a

# enable warnings and turn them into errors to enforce warning free code
AM_CFLAGS = -Wall -Werror

libsidutils_a_CFLAGS = $(AM_CFLAGS)

libsidutils_a_SOURCES = SidDatabase.cpp SidTuneMod.cpp
