# SPDX-License-Identifier: GPL-2.0-or-later

EXTRA_DIST = stilview/README.txt stilview/STILView-config.patch

lib_LIBRARIES = libpsid64.a

libpsid64_a_CFLAGS = $(AM_CFLAGS)

libpsid64_a_SOURCES = \
	psid64.cpp \
	psidboot.a65 \
	psidboot.h \
	psidextboot.h \
	psiddrv.a65 \
	psiddrv.h \
	psidextdrv.h \
	reloc65.cpp \
	reloc65.h \
	screen.cpp \
	screen.h \
	sidid.cpp \
	sidid.h \
	theme.cpp \
	theme.h \
	exomizer/chunkpool.c \
	exomizer/chunkpool.h \
	exomizer/exomizer.c \
	exomizer/exomizer.h \
	exomizer/log.h \
	exomizer/match.c \
	exomizer/match.h \
	exomizer/optimal.c \
	exomizer/optimal.h \
	exomizer/output.c \
	exomizer/output.h \
	exomizer/radix.c \
	exomizer/radix.h \
	exomizer/search.c \
	exomizer/search.h \
	exomizer/sfx64ne.c \
	exomizer/sfx.h \
	stilview/stil.cpp \
	stilview/stil.h \
	stilview/stilcomm.cpp \
	stilview/stilcomm.h \
	stilview/stildefs.h

BUILT_SOURCES = \
	psidboot.h \
	psidextboot.h \
	psiddrv.h \
	psidextdrv.h

SUFFIXES = .a65

.h : .a65
	xa -R -G $< -o $(srcdir)/$*.o65
	od -v -An -w8 -tx1 $(srcdir)/$*.o65 | sed -re 's/[[:alnum:]]+/0x&,/g' > $(srcdir)/$@

psidboot.h: psidboot.a65
	xa -c -R -G $< -o - -DSMOOTH_SCROLL | od -v -An -tx1 | sed -re 's/[[:alnum:]]+/0x&,/g' -e 's/[[:space:]]+//g' > $(srcdir)/$@

psidextboot.h: psidboot.a65
	xa -c -R -G $< -o - -DSCREEN -DSMOOTH_SCROLL | od -v -An -tx1 | sed -re 's/[[:alnum:]]+/0x&,/g' -e 's/[[:space:]]+//g' > $(srcdir)/$@

psiddrv.h: psiddrv.a65
	xa -c -R -G $< -o - | od -v -An -tx1 | sed -re 's/[[:alnum:]]+/0x&,/g' -e 's/[[:space:]]+//g' > $(srcdir)/$@

psidextdrv.h: psiddrv.a65
	xa -c -R -G $< -o - -DSCREEN -DSMOOTH_SCROLL | od -v -An -tx1 | sed -re 's/[[:alnum:]]+/0x&,/g' -e 's/[[:space:]]+//g' > $(srcdir)/$@
