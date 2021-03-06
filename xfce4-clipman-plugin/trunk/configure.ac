dnl configure.ac
dnl
dnl xfce4-clipman-plugin - Clipboard manager for the Xfce desktop
dnl
dnl 2005 Eduard Roccatello <eduard@xfce.org>
dnl 2006 Nick Schermer <nick@xfce.org>
dnl

dnl version info
m4_define([clipman_version], [0.6.4])

dnl init autoconf
AC_INIT([xfce4-clipman-plugin], [clipman_version], [nickschermer@gmail.com])
AC_PREREQ([2.50])

dnl init automake
CLIPMAN_VERSION=clipman_version
AM_INIT_AUTOMAKE([xfce4-clipman-plugin], [$CLIPMAN_VERSION])
AM_CONFIG_HEADER([config.h])
AM_MAINTAINER_MODE

dnl check for basic programs
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_INTLTOOL()

dnl check for standard header files
AC_HEADER_STDC

dnl Check for i18n support
XDT_I18N([ja hu
	  ])

dnl Check for X11 installed
XDT_CHECK_LIBX11_REQUIRE
XDT_CHECK_LIBSM

dnl required
XDT_CHECK_PACKAGE([GTHREAD], [gthread-2.0], [2.6.0])
XDT_CHECK_PACKAGE([GTK], [gtk+-2.0], [2.6.0])
XDT_CHECK_PACKAGE([LIBXFCEGUI4], [libxfcegui4-1.0], [4.3.90])
XDT_CHECK_PACKAGE([LIBXFCE4UTIL], [libxfce4util-1.0], [4.3.90])
XDT_CHECK_PACKAGE([LIBXFCE4PANEL], [libxfce4panel-1.0], [4.3.90])

AC_MSG_CHECKING([whether to use new header dialog])
if $PKG_CONFIG --atleast-version 4.3.90.2 libxfcegui4-1.0 >/dev/null 2>&1; then
  AC_DEFINE([USE_NEW_DIALOG], [1], [Define to use new header dialog])
  AC_MSG_RESULT([yes])
else
  AC_MSG_RESULT([no])
fi

dnl check for debugging support
XDT_FEATURE_DEBUG()

AC_OUTPUT([
Makefile
panel-plugin/Makefile
po/Makefile.in
])
