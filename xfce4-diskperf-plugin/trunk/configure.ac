dnl configure.ac
dnl
dnl xfce4-diskperf-plugin - Disk performance plugin for xfce4-panel
dnl
dnl 2003, 2004 Roger Seguin <roger_seguin@msn.com>
dnl

m4_define([diskperf_version],[2.0])

AC_INIT([xfce4-diskperf-plugin], [diskperf_version()],
	[xfce-goodies-dev@lists.berlios.de])

DISKPERF_VERSION=diskperf_version()
AM_INIT_AUTOMAKE([xfce4-diskperf-plugin], [$DISKPERF_VERSION])
AM_CONFIG_HEADER([config.h])

AM_MAINTAINER_MODE

dnl Check for UNIX variants
AC_AIX
AC_ISC_POSIX
AC_MINIX

dnl Check for basic programs
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_INTLTOOL

dnl Check for standard header files
AC_HEADER_STDC

dnl configure the panel plugin
XDT_CHECK_PACKAGE([LIBXFCE4PANEL], [libxfce4panel-1.0], [4.3.22])

dnl Translations
XDT_I18N([])

dnl Check for debugging support
XDT_FEATURE_DEBUG()

AC_OUTPUT([
Makefile
panel-plugin/Makefile
po/Makefile.in
])
