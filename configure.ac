
dnl --------------------------------
dnl Initialization macros.
dnl --------------------------------

AC_INIT(libacovea/acovea.h)

dnl -----------------------------------------------
dnl Package name and version number (user defined)
dnl -----------------------------------------------

GENERIC_LIBRARY_NAME=libacovea

#release versioning
GENERIC_MAJOR_VERSION=5
GENERIC_MINOR_VERSION=1
GENERIC_MICRO_VERSION=1

#shared PROGRAM versioning
GENERIC_LIBRARY_VERSION=5:3:0
#                       | | |
#                +------+ | +---+
#                |        |     |
#             current:revision:age
#                |        |     |
#                |        |     +- increment if interfaces have been added
#                |        |        set to zero if interfaces have been removed
#                                  or changed
#                |        +- increment if source code has changed
#                |           set to zero if current is incremented
#                +- increment if interfaces have been added, removed or changed


dnl --------------------------------
dnl Package name and version number
dnl --------------------------------

AC_SUBST(GENERIC_LIBRARY_VERSION)

PACKAGE=$GENERIC_LIBRARY_NAME
AC_SUBST(GENERIC_LIBRARY_NAME)

GENERIC_VERSION=$GENERIC_MAJOR_VERSION.$GENERIC_MINOR_VERSION.$GENERIC_MICRO_VERSION
GENERIC_RELEASE=$GENERIC_MAJOR_VERSION.$GENERIC_MINOR_VERSION
AC_SUBST(GENERIC_RELEASE)
AC_SUBST(GENERIC_VERSION)

VERSION=$GENERIC_VERSION

AM_INIT_AUTOMAKE($PACKAGE, $VERSION, no-define)

dnl -----------------------------------------------
dnl Checks for programs.
dnl -----------------------------------------------

AC_PROG_CC
AC_PROG_CXX
AM_PROG_LIBTOOL
AM_SANITY_CHECK
AC_LANG_CPLUSPLUS

dnl -----------------------------------------------
dnl Checks for required headers
dnl -----------------------------------------------

AC_HEADER_STDC
AC_HEADER_STDBOOL
AC_CHECK_HEADERS(stdint.h unistd.h fcntl.h)

dnl -----------------------------------------------
dnl Checks for libraries.
dnl -----------------------------------------------

dnl AC_CHECK_LIB(coyotl, sigdig)
AC_CHECK_HEADER(libcoyotl/prng.h)
AC_CHECK_HEADER(libcoyotl/mwc1038.h)
AC_CHECK_HEADER(libcoyotl/mtwister.h)
AC_CHECK_HEADER(libevocosm/evocosm.h)
AC_CHECK_HEADER(expat.h)

dnl -----------------------------------------------
dnl Generates Makefile's, configuration files and scripts
dnl -----------------------------------------------

AC_OUTPUT(Makefile acovea.pc libacovea/Makefile cmdline/Makefile)

