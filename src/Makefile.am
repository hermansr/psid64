bin_PROGRAMS = psid64

psid64_SOURCES = \
	datatypes.h \
	main.c \
	psid.c \
	psid.h \
	psidboot.a65 \
	psidboot.h \
	psiddrv.a65 \
	psiddrv.h \
	psidextdrv.h \
	reloc65.c \
	reloc65.h \
	screen.c \
	screen.h

BUILT_SOURCES = \
	psidboot.h \
	psiddrv.h \
	psidextdrv.h

psid64_LDADD = @GLIB_LIBS@

INCLUDES = @GLIB_CFLAGS@

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

# show all warnings
AM_CFLAGS += -Wall
