dnl Process this file with autoconf to produce a configure script.
AC_INIT(src/main.c)

dnl Checks for programs.

dnl Checks for libraries.


AM_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE([xfce4-bglist-editor], [4.1.9])
AM_MAINTAINER_MODE

dnl Check for UNIX variants
AC_AIX
AC_ISC_POSIX
AC_MINIX

dnl Check for basic programs
AC_PROG_CC
AC_PROG_INSTALL

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h limits.h sys/ioctl.h])

dnl AC_CONFIG_HEADERS([config.h])

AC_DISABLE_STATIC

AC_PROG_LIBTOOL


dnl Check for i18n support
BM_I18N([xfce4-bglist-editor], [de])

dnl BM_DEPEND([XML], [libxml-2.0], [2.4.0])
BM_DEPEND([XFCE4_UTIL], [libxfce4util-1.0], [4.0.0])
BM_DEPEND([XFCE4_GUI], [libxfcegui4-1.0], [4.0.0])
BM_DEPEND([GTK], [gtk+-2.0], [2.2.6])
dnl BM_DEPEND([XFCE_MCS_CLIENT], [libxfce4mcs-client-1.0], [4.0.0])
dnl XFCE_PANEL_PLUGIN([XFCE4_PANEL], [4.0.0])
dnl XFCE_MCS_PLUGIN([XFCE_MCS_MANAGER], [4.0.0])

dnl Check for debugging support
dnl undefined symbol?
dnl BM_DEBUG_SUPPORT

dnl Checks for library functions.
dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

AC_OUTPUT([
Makefile
src/Makefile
])
