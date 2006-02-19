dnl configure.ac
dnl
dnl xfce4-radio-plugin - <Plugin for xfce4-panel>
dnl
dnl 2006 Stefan Ott <stefan@desire.ch>
dnl

m4_define([radio_version],[0.1.0])

AC_INIT([xfce4-radio-plugin], [radio_version()],
	[xfce-goodies-dev@lists.berlios.de])

RADIO_VERSION=radio_version()
AM_INIT_AUTOMAKE([xfce4-radio-plugin], [$RADIO_VERSION])
AM_CONFIG_HEADER([config.h])

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
XFCE_PANEL_PLUGIN([XFCE4_PANEL], [4.0.0])

dnl Check for debugging support
BM_DEBUG_SUPPORT

AC_OUTPUT([
Makefile
panel-plugin/Makefile
icons/Makefile
])
