dnl Process this file with autoconf to produce a configure script.
AC_INIT(src/main.cpp)
AM_INIT_AUTOMAKE(psid64, 0.7)
AM_CONFIG_HEADER(src/config.h)

dnl Checks for programs.
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_RANLIB

dnl Checks for libraries.
dnl AM_PATH_GLIB(1.2.2,,AC_MSG_ERROR([*** GLIB >= 1.2.2 not installed - please install first ***]))

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([getopt.h limits.h stdlib.h string.h unistd.h])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T

dnl Checks for library functions.
AC_FUNC_MEMCMP
AC_CHECK_FUNCS([getopt_long memset strncasecmp strcasecmp strchr strndup strrchr strstr snprintf])

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
AC_CREATE_STDINT_H(include/sidplay/sidint.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_BIGENDIAN
if test $ac_cv_c_bigendian = yes; then
    MY_SUBST(SID_WORDS_ENDIANESS,SID_WORDS_BIGENDIAN)
else
    MY_SUBST(SID_WORDS_ENDIANESS,SID_WORDS_LITTLEENDIAN)
fi

AC_CHECK_SIZEOF(char, 1)
AC_CHECK_SIZEOF(short int, 2)
AC_CHECK_SIZEOF(int, 4)
AC_CHECK_SIZEOF(long int, 4)

MY_SUBST(SID_SIZEOF_CHAR,$ac_cv_sizeof_char)
MY_SUBST(SID_SIZEOF_SHORT_INT,$ac_cv_sizeof_short_int)
MY_SUBST(SID_SIZEOF_INT,$ac_cv_sizeof_int)
MY_SUBST(SID_SIZEOF_LONG_INT,$ac_cv_sizeof_long_int)

dnl Use C++ for other tests.
AC_LANG(C++)

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

ACLOCAL_AMFLAGS="-I macros"
AC_SUBST(ACLOCAL_AMFLAGS)

dnl
dnl END_SIDTUNE_TESTS
dnl

AC_CONFIG_FILES([Makefile
                 psid64.spec
                 include/sidplay/sidconfig.h
                 src/Makefile
                 src/libpsid64/Makefile
                 src/sidtune/Makefile
                 src/sidutils/Makefile
		 src/sidutils/MD5/Makefile
		 src/sidutils/ini/Makefile])
AC_OUTPUT
