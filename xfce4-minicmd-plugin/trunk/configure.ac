dnl configure.ac
dnl
dnl

AC_INIT([panel-plugin/command.c])

AM_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE([xfce4-minicmd-plugin], [0.3.0])

AM_MAINTAINER_MODE

dnl Check for UNIX variants
AC_AIX
AC_ISC_POSIX
AC_MINIX

dnl Check for basic programs
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

dnl Check for standard header files
AC_HEADER_STDC

dnl configure the panel plugin
XFCE_PANEL_PLUGIN([XFCE4_PANEL], [4.1.90])

dnl link to libxfce4util
PKG_CHECK_MODULES(LIB_XFCE4_UTIL, libxfce4util-1.0)

dnl Check for debugging support
BM_DEBUG_SUPPORT

AC_OUTPUT([
Makefile
panel-plugin/Makefile
])
