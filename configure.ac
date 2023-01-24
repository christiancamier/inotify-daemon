#                                               -*- Autoconf -*-
# 
# Copyright (c) 2022
# 	Christian CAMIER <christian.c@promethee.services>
# 
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
# 
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
# 

AC_PREREQ([2.69])
AC_INIT([inotify-daemon], [0.1], [christian.c@promethee.services])
AC_CONFIG_SRCDIR([src/inotify-daemon.c])
AC_CONFIG_HEADERS([src/ind_config.h])

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

# Gets the libCCR needed for compilation
if   test -d libCCR
then :
else
  AC_MSG_NOTICE("Getting LibCCR")
  git clone https://github.com/christiancamier/libCCR.git || AC_MSG_ERROR([Cannot retrieve libCCR from github])
  chmod +x libCCR/configure                               || AC_MSG_ERROR([There is a problem with libCCR/configure])
fi

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

AC_CONFIG_SUBDIRS([libCCR])
AC_OUTPUT
