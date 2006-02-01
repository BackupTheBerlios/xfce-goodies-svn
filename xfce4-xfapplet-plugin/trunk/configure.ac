dnl configure.ac
dnl xfce4-xfapplet-plugin: a plugin for the Xfce 4 panel capable of displaying
dnl			   Gnome applets.

dnl version info
m4_define([xfapplet_version], [0.0.1])

dnl init autoconf
AC_INIT([xfce4-xfapplet-plugin], [xfapplet_version], [awbess@gmail.com])
AC_PREREQ([2.50])

dnl init automake
XFAPPLET_VERSION=xfapplet_version
AM_INIT_AUTOMAKE([dist-bzip2])
AM_CONFIG_HEADER([config.h])
AM_MAINTAINER_MODE

dnl check for UNIX variants
AC_AIX
AC_ISC_POSIX
AC_MINIX
AM_CONDITIONAL([HAVE_CYGWIN], [test "`uname | grep \"CYGWIN\"`" != ""])

dnl check for basic programs
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_INTLTOOL([0.31], [no-xml])

dnl check for standard header files
AC_HEADER_STDC

dnl Check for i18n support
XDT_I18N([])

dnl Check for X11 installed
XDT_CHECK_LIBX11_REQUIRE
XDT_CHECK_LIBSM

dnl required
XDT_CHECK_PACKAGE([GOBJECT], [gobject-2.0], [2.4.0])
XDT_CHECK_PACKAGE([GTK], [gtk+-2.0], [2.4.0])
XDT_CHECK_PACKAGE([LIBBONOBOUI], [libbonoboui-2.0], [2.1.1])
XDT_CHECK_PACKAGE([LIBXFCE4PANEL], [libxfce4panel-1.0], [4.3.20])
XDT_CHECK_PACKAGE([LIBXFCE4UTIL], [libxfce4util-1.0], [4.2.0])

dnl check for debugging support
XDT_FEATURE_DEBUG

AC_OUTPUT([
Makefile
panel-plugin/Makefile
po/Makefile.in
])
