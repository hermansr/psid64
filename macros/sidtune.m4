AC_DEFUN(MY_SUBST_DEF,
[
    eval "$1=\"#define $1\""
    AC_SUBST($1)
])

AC_DEFUN(MY_SUBST_UNDEF,
[
    eval "$1=\"#undef $1\""
    AC_SUBST($1)
])

AC_DEFUN(MY_SUBST,
[
    eval "$1=$2"
    AC_SUBST($1)
])





dnl -------------------------------------------------------------------------
dnl Check whether compiler has a working ``bool'' type.
dnl Will substitute @HAVE_BOOL@ with either a def or undef line.
dnl -------------------------------------------------------------------------

AC_DEFUN(MY_CHECK_BOOL,
[
    AC_MSG_CHECKING([for bool])
    AC_CACHE_VAL(test_cv_have_bool,
    [
        AC_TRY_COMPILE(
            [],
            [bool aBool = true;],
            [test_cv_have_bool=yes],
            [test_cv_have_bool=no]
        )
    ])
    AC_MSG_RESULT($test_cv_have_bool)
    if test "$test_cv_have_bool" = yes; then
        AC_DEFINE(HAVE_BOOL,,[Define if the C++ compiler supports BOOL])
    fi
])

dnl -------------------------------------------------------------------------
dnl Check whether C++ library has member ios::bin instead of ios::binary.
dnl Will substitute @HAVE_IOS_BIN@ with either a def or undef line.
dnl -------------------------------------------------------------------------

AC_DEFUN(MY_CHECK_IOS_BIN,
[
    AC_MSG_CHECKING([whether standard member ios::binary is available])
    AC_CACHE_VAL(test_cv_have_ios_binary,
    [
        AC_TRY_COMPILE(
            [#include <fstream.h>],
            [ifstream myTest(ios::in|ios::binary);],
            [test_cv_have_ios_binary=yes],
            [test_cv_have_ios_binary=no]
        )
    ])
    AC_MSG_RESULT($test_cv_have_ios_binary)
    if test "$test_cv_have_ios_binary" = yes; then
        AC_DEFINE(HAVE_IOS_BIN,,
            [Define if standard member ``ios::binary'' is called ``ios::bin''.]
        )
    fi
])

dnl -------------------------------------------------------------------------
dnl Check whether C++ library has member ios::bin instead of ios::binary.
dnl Will substitute @HAVE_IOS_OPENMODE@ with either a def or undef line.
dnl -------------------------------------------------------------------------

AC_DEFUN(MY_CHECK_IOS_OPENMODE,
[
    AC_MSG_CHECKING([whether standard member ios::openmode is available])
    AC_CACHE_VAL(test_cv_have_ios_openmode,
    [
        AC_TRY_COMPILE(
            [#include <fstream.h>
             #include <iomanip.h>],
            [ios::openmode myTest = ios::in;],
            [test_cv_have_ios_openmode=yes],
            [test_cv_have_ios_openmode=no]
        )
    ])
    AC_MSG_RESULT($test_cv_have_ios_openmode)
    if test "$test_cv_have_ios_openmode" = yes; then
        AC_DEFINE(HAVE_IOS_OPENMODE,,
            [Define if ``ios::openmode'' is supported.]
        )
    fi
])

dnl -------------------------------------------------------------------------
dnl Check whether C++ environment provides the "nothrow allocator".
dnl Will substitute @HAVE_EXCEPTIONS@ if test code compiles.
dnl -------------------------------------------------------------------------

AC_DEFUN(MY_CHECK_EXCEPTIONS,
[
    AC_MSG_CHECKING([whether exceptions are available])
    AC_CACHE_VAL(test_cv_have_exceptions,
    [
        AC_TRY_COMPILE(
            [#include <new.h>],
            [char* buf = new(nothrow) char[1024];],
            [test_cv_have_exceptions=yes],
            [test_cv_have_exceptions=no]
        )
    ])
    AC_MSG_RESULT($test_cv_have_exceptions)
    if test "$test_cv_have_exceptions" = yes; then
        AC_DEFINE(HAVE_EXCEPTIONS,,
            [Define if your C++ compiler implements exception-handling.]
        )
    fi
])
