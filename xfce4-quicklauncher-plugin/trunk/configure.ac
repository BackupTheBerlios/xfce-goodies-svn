dnl 
dnl 	configure.ac
dnl	quicklauncher plugin 
dnl	2005 Masse Nicolas (masse_nicolas@yahoo.fr)
dnl

AC_INIT([configure])

AM_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE([xfce4-Quicklauncher-plugin], [0.6.0])

AM_MAINTAINER_MODE

dnl Check for UNIX variants
AC_AIX
AC_ISC_POSIX
AC_MINIX
AM_CONDITIONAL([HAVE_CYGWIN], [test "`uname | grep \"CYGWIN\"`" != ""])

dnl Check for basic programs
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

dnl Check for standard header files
AC_HEADER_STDC

dnl configure the panel plugin
XFCE_PANEL_PLUGIN([XFCE4_PANEL], [4.1.9.0])

dnl Check for debugging support
BM_DEBUG_SUPPORT

AC_OUTPUT([
Makefile
panel-plugin/Makefile
])
