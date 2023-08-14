# SPDX-License-Identifier: GPL-2.0-or-later

EXTRA_DIST = stilview/README.txt stilview/STILView-config.patch

AM_CFLAGS = $(WARNINGCFLAGS)
AM_CXXFLAGS = $(WARNINGCXXFLAGS)

lib_LIBRARIES = libpsid64.a

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

define my_xa_recipe
set -o pipefail ; \
if xa -M -c -R -G $< -o - $(1) | \
od -v -An -tx1 | \
sed -re 's/[[:alnum:]]+/0x&,/g' -e 's/[[:space:]]+//g' -e '/^$$/d' > $(srcdir)/$@ ; \
then true ; \
else status=$$? ; $(RM) $(srcdir)/$@ ; exit $$status ; \
fi
endef

define my_xa_recipe_ext
set -o pipefail ; \
if xa -M -c -R -G $< -o - -DSCREEN -DSMOOTH_SCROLL | \
od -v -An -tx1 | \
sed -re 's/[[:alnum:]]+/0x&,/g' -e 's/[[:space:]]+//g' -e '/^$$/d' > $(srcdir)/$@ ; \
then true ; \
else status=$$? ; $(RM) $(srcdir)/$@ ; exit $$status ; \
fi
endef

SUFFIXES = .a65

.a65.h:
	$(my_xa_recipe)

psidextboot.h: psidboot.a65
	$(my_xa_recipe_ext)

psidextdrv.h: psiddrv.a65
	$(my_xa_recipe_ext)
