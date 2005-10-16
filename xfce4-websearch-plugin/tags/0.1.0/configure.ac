dnl $Id$

m4_define([websearch_version_major], [0])
m4_define([websearch_version_minor], [1])
m4_define([websearch_version_micro], [0])
m4_define([websearch_version],
    [websearch_version_major.websearch_version_minor.websearch_version_micro])

AC_COPYRIGHT([Copyright (c) 2005 by Piotr Wolny <gildur@gmail.com>])
AC_INIT([xfce4-websearch-plugin], [websearch_version], [gildur@gmail.com])

AM_INIT_AUTOMAKE([AC_PACKAGE_TARNAME], [AC_PACKAGE_VERSION])
AM_CONFIG_HEADER([config.h])
AM_MAINTAINER_MODE

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

AC_HEADER_STDC

XDT_I18N([pl de])

XFCE_PANEL_PLUGIN([XFCE4_PANEL], [4.2.0])

AC_OUTPUT([Makefile
           po/Makefile.in
           src/Makefile])
