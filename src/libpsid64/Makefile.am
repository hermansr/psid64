EXTRA_DIST = stilview/README.txt stilview/STILView-config.patch

lib_LIBRARIES = libpsid64.a

libpsid64_a_SOURCES = \
	psid64.cpp \
	psidboot.a65 \
	psidboot.h \
	psiddrv.a65 \
	psiddrv.h \
	psidextdrv.h \
	reloc65.cpp \
	reloc65.h \
	screen.cpp \
	screen.h \
	stilview/stil.cpp \
	stilview/stil.h \
	stilview/stilcomm.cpp \
	stilview/stilcomm.h \
	stilview/stildefs.h

BUILT_SOURCES = \
	psidboot.h \
	psiddrv.h \
	psidextdrv.h

SUFFIXES = .a65

%.h : %.a65
	xa -R -G $< -o $(srcdir)/$*.o65
	od -v -An -w8 -tx1 $(srcdir)/$*.o65 | sed 's/\w\w/0x&,/g' > $(srcdir)/$@

psidboot.h: psidboot.a65
	xa $< -o - | od -v -An -w8 -tx1 | sed 's/\w\w/0x&,/g' > $(srcdir)/$@

psiddrv.h: psiddrv.a65
	xa -R -G $< -o - | od -v -An -w8 -tx1 | sed 's/\w\w/0x&,/g' > $(srcdir)/$@

psidextdrv.h: psiddrv.a65
	xa -R -G $< -o - -DSCREEN | od -v -An -w8 -tx1 | sed 's/\w\w/0x&,/g' > $(srcdir)/$@
