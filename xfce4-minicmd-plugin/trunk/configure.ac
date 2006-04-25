dnl configure.ac
dnl
dnl

m4_define([minicmd_version],[0.4.0])

AC_INIT([xfce4-minicmd-plugin],[minicmd_version()],
	[xfce-goodies-dev@lists.berlios.de])

MINICMD_VERSION=minicmd_version()
AM_INIT_AUTOMAKE([xfce4-minicmd-plugin], [$MINICMD_VERSION])
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
AC_PROG_LIBTOOL

dnl Check for standard header files
AC_HEADER_STDC

dnl configure the panel plugin
XDT_CHECK_PACKAGE([LIBXFCE4PANEL],[libxfce4panel-1.0],[4.3.20])

dnl Translations
XDT_I18N()

dnl Check for debugging support
XDT_FEATURE_DEBUG()

AC_OUTPUT([
Makefile
panel-plugin/Makefile
po/Makefile.in
])
