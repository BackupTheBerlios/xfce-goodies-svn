dnl configure.ac
dnl
dnl xfce4-screenshooter-plugin - ScreenShooter plugin for xfce4-panel
dnl
dnl 2005 Daniel Bobadilla Leal <dbobadil@dcc.uchile.cl>
dnl

AC_INIT([panel-plugin/shooter.c])

AM_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE([xfce4-screenshooter-plugin], [0.0.7])

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
XFCE_PANEL_PLUGIN([XFCE4_PANEL], [0.10.1])

dnl Check for debugging support
BM_DEBUG_SUPPORT

AC_OUTPUT([
Makefile
panel-plugin/Makefile
])
