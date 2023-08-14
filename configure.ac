dnl Process this file with autoconf to produce a configure script.
AC_INIT([psid64],[1.3],,,[https://www.psid64.org/])
AC_CONFIG_SRCDIR(src/main.cpp)
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_MACRO_DIRS([macros])
AM_INIT_AUTOMAKE([subdir-objects])

dnl Checks for programs.
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_RANLIB

dnl Checks for header files.
AC_CHECK_HEADERS([getopt.h limits.h stdlib.h string.h unistd.h])
AC_CHECK_HEADER_STDBOOL

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT8_T

dnl Checks for library functions.
AC_FUNC_MEMCMP
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_CHECK_FUNCS([getopt_long lstat memmove memset mkdir snprintf strcasecmp strchr strdup strerror strncasecmp strrchr strstr])
AX_FUNC_MKDIR

dnl
dnl BEGIN_SIDTUNE_TESTS
dnl

dnl Check for AC99 headers
AC_CHECK_HEADERS(stdbool.h)
if test "$ac_cv_header_stdbool_h" = yes; then
    MY_SUBST_DEF(SID_HAVE_STDBOOL_H)
else
    MY_SUBST_UNDEF(SID_HAVE_STDBOOL_H)
fi
AX_CREATE_STDINT_H(include/sidplay/sidint.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_BIGENDIAN(
    [MY_SUBST(SID_WORDS_ENDIANESS,SID_WORDS_BIGENDIAN)],
    [MY_SUBST(SID_WORDS_ENDIANESS,SID_WORDS_LITTLEENDIAN)]
)

AC_CHECK_SIZEOF(char)
AC_CHECK_SIZEOF(short int)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long int)

MY_SUBST(SID_SIZEOF_CHAR,$ac_cv_sizeof_char)
MY_SUBST(SID_SIZEOF_SHORT_INT,$ac_cv_sizeof_short_int)
MY_SUBST(SID_SIZEOF_INT,$ac_cv_sizeof_int)
MY_SUBST(SID_SIZEOF_LONG_INT,$ac_cv_sizeof_long_int)

dnl Use C++ for other tests.
AC_LANG(C++)

dnl Checks for C++ library functions.
AC_CHECK_HEADER(sstream,[AC_DEFINE(HAVE_SSTREAM,,
  [Define if you have the <sstream> header file.])],[])

dnl Check whether the compiler has bool.
MY_CHECK_BOOL
if test "$test_cv_have_bool" = yes; then
    MY_SUBST_DEF(SID_HAVE_BOOL)
else
    MY_SUBST_UNDEF(SID_HAVE_BOOL)
fi

dnl Check for exceptions.
MY_CHECK_EXCEPTIONS

dnl Check ANSI C++ ios features
MY_CHECK_IOS_BIN
MY_CHECK_IOS_OPENMODE

dnl Add absolute directory for header files
CXXFLAGS="$CXXFLAGS -I\$(top_srcdir)/include -I\$(top_srcdir)/include/sidplay\
 -I\$(top_srcdir)/include/sidplay/utils"

dnl
dnl END_SIDTUNE_TESTS
dnl

dnl enable warnings and turn them into errors to enforce warning free code
AC_SUBST([WARNINGCFLAGS],['-Wall -Werror'])
AC_SUBST([WARNINGCXXFLAGS],['-Wall -Werror'])

AC_CONFIG_FILES([Doxyfile
                 Makefile
                 psid64.spec
                 include/sidplay/sidconfig.h
                 src/Makefile
                 src/libpsid64/Makefile
                 src/sidtune/Makefile
                 src/sidutils/Makefile
                 src/sidutils/MD5/Makefile
                 src/sidutils/ini/Makefile])
AC_OUTPUT
