#                                               -*- Autoconf -*-
#
# Copyright: Christian CAMIER & Quentin PERIDON 2022
#
# christian.c@promethee.services
#
# This software is a computer program whose purpose is to manage filesystems
# events on defined directories.
#
# This software is governed by the CeCILL-B license under French law and
# abiding by the rules of distribution of free software.  You can  use, 
# modify and/ or redistribute the software under the terms of the CeCILL-B
# license as circulated by CEA, CNRS and INRIA at the following URL
# "http://www.cecill.info". 
# 
# As a counterpart to the access to the source code and  rights to copy,
# modify and redistribute granted by the license, users are provided only
# with a limited warranty  and the software's author,  the holder of the
# economic rights,  and the successive licensors  have only  limited
# liability. 
#
# In this respect, the user's attention is drawn to the risks associated
# with loading,  using,  modifying and/or developing or reproducing the
# software by the user in light of its specific status of free software,
# that may mean  that it is complicated to manipulate,  and  that  also
# therefore means  that it is reserved for developers  and  experienced
# professionals having in-depth computer knowledge. Users are therefore
# encouraged to load and test the software's suitability as regards their
# requirements in conditions enabling the security of their systems and/or 
# data to be ensured and,  more generally, to use and operate it in the 
# same conditions as regards security. 
#
# The fact that you are presently reading this means that you have had
# knowledge of the CeCILL-B license and that you accept its terms.
#

AC_PREREQ([2.69])
AC_INIT([inotify-daemon], [0.1], [christian.c@promethee.services])
AC_CONFIG_SRCDIR([src/ind_main.c])
AC_CONFIG_HEADERS([src/ind_config.h])

AC_ARG_ENABLE([debug], [Insert full debugging traces], [AC_DEFINE([DEBUG], [], [Undef for debugging])])

# AC_ARG_WITH([package], AS_HELP_STRING([--with-package], [Do not install, create package]))

AC_MSG_CHECKING([Operating system name])
OS_NAME=`uname -s`
test "$OS_NAME" == 'Linux' || AC_MSG_ERROR "Operatings system '$OS_NAME' is not supported"

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

AC_CHECK_PROG(HAVE_BASH, [bash], [yes], [no])
test "$HAVE_BASH" == "no" && AC_MSG_ERROR([Required programm bash not found])

AC_CHECK_PROG(HAVE_GIT, [git], [yes], [no])
test "$HAVE_GIT" == "no" && AC_MSG_ERROR([Required programm git not found])

AC_CHECK_PROG(HAVE_PYTHON3, [python3], [yes], [no])
test "$HAVE_PYTHON3" == "no" && AC_MSG_ERROR([Required programm python3 not found])

AC_CHECK_PROG(HAVE_TEXI2ANY, [texi2any], [yes], [no])
AC_SUBST(have_texi2any, $HAVE_TEXI2ANY)
test "$HAVE_TEXI2ANY" == "non" && AC_MSG_NOTICE("Info documentation will not be built")

AC_CHECK_PROG(HAVE_TEXI2PDF, [texi2pdf], [yes], [no])
AC_SUBST(have_texi2pdf, $HAVE_TEXI2PDF)
test "$HAVE_TEXI2ANY" == "non" && AC_MSG_NOTICE("PDF documentation will not be built")

ccr_os_pkg_type=''
ccr_os_id=''
if   test -f /etc/os-release
then eval `sed -e 's/^/ccr_os_/g' -e 's/$/;/g' /etc/os-release`
elif test -f /usr/lib/os-release
then eval `sed -e 's/^/ccr_os_/g' -e 's/$/;/g' /usr/lib/os-release`
fi

case "${ccr_os_ID}" in
debian|ubuntu)
  AC_CHECK_PROG(HAVE_DPKG_DEB, [dpkg-deb], [true], [false])
  $HAVE_DPKG_DEB && ccr_os_pkg_type=deb
  ;;
centos|fedora|rhel)
  AC_CHECK_PROG(HAVE_RPMBUILD, [rpmbuild], [true], [false])
  $HAVE_RPMBUILD && ccr_os_pkg_type=rpm
  ;;
esac

AC_SUBST(pkgdir, $ccr_os_pkg_type)

# Makefiles substitutions
AC_SUBST(topdir, `pwd`)

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h paths.h stdint.h stdlib.h string.h strings.h sys/param.h syslog.h unistd.h])
AC_CHECK_HEADERS([sys/types.h sys/inotify.h sys/signalfd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT32_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_CHECK_FUNCS([atexit dup2 endgrent endpwent inotify_add_watch inotify_init1 memset mkdir signalfd strcasecmp strdup strerror strrchr strtol])

AC_CONFIG_FILES([Makefile
		 Makefile.inc
                 src/Makefile
		 doc/Makefile
		 doc/Makefile.inc
                 doc/en/Makefile
		 doc/fr/Makefile
		 deb/Makefile
		 rpm/Makefile])

AC_OUTPUT
