
dnl --------------------------------
dnl Initialization macros.
dnl --------------------------------

AC_INIT([libacovea], [5.1.2])
AC_CONFIG_SRCDIR([libacovea/acovea.h])
AC_CONFIG_MACRO_DIR([m4])

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

AM_INIT_AUTOMAKE([1.15 foreign dist-bzip2 no-dist-gzip subdir-objects -Wall])

dnl -----------------------------------------------
dnl Checks for programs.
dnl -----------------------------------------------

AC_PROG_CXX
AM_PROG_AR
LT_INIT([shared disable-static])

dnl -----------------------------------------------
dnl Checks for libraries.
dnl -----------------------------------------------

AC_LANG_PUSH([C++])

# libcoyotl
AC_CHECK_HEADER([libcoyotl/prng.h])
AC_CHECK_HEADER([libcoyotl/mwc1038.h])
AC_CHECK_HEADER([libcoyotl/mtwister.h])

save_LIBS="${LIBS}"
LIBS="${LIBS} -lcoyotl"
AC_LINK_IFELSE([
  AC_LANG_PROGRAM(
    [[#include <libcoyotl/command_line.h>]],
    [[libcoyotl::command_line cl_(0, NULL, std::set<std::string>());]]
  )
], [
  LIBCOYOTL_LIBS="-lcoyotl"
  AC_SUBST([LIBCOYOTL_LIBS])
], [
  AC_MSG_ERROR([Could not find libcoyotl!])
])
LIBS="${save_LIBS}"

# libevocosm
AC_CHECK_HEADER([libevocosm/evocosm.h])

save_LIBS="${LIBS}"
LIBS="${LIBS} -levocosm"
AC_LINK_IFELSE([
  AC_LANG_PROGRAM(
    [[#include <libevocosm/evocosm.h>]],
    [[uint32_t test_ = libevocosm::globals::get_seed();]]
  )
], [
  LIBEVOCOSM_LIBS="-levocosm"
  AC_SUBST([LIBEVOCOSM_LIBS])
], [
  AC_MSG_ERROR([Could not find libevocosm!])
])
LIBS="${save_LIBS}"
AC_LANG_POP

# brahe
AC_SEARCH_LIBS([brahe_prng_init], [brahe])

# expat
PKG_CHECK_MODULES([EXPAT], [expat])

dnl -----------------------------------------------
dnl Set up pkg-config installation dir
dnl -----------------------------------------------

AX_REQUIRE_DEFINED([PKG_INSTALLDIR])
PKG_INSTALLDIR

dnl -----------------------------------------------
dnl Generates Makefile's, configuration files and scripts
dnl -----------------------------------------------

AC_OUTPUT([
  Makefile
  acovea.pc])
