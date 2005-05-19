dnl configure.ac
dnl
dnl xfce4-modem-lights-plugin - a modem dialing plugin for the xfce4 panel
dnl

dnl version info
m4_define([modem_lights_version], [0.1.2])

dnl init autoconf
AC_INIT([xfce4-modemlights-plugin], [modem_lights_version],
	[aguelzow@taliesin.ca])
AC_PREREQ([2.50])

dnl init automake
MODEM_LIGHTS_VERSION=modem_lights_version
AM_INIT_AUTOMAKE([xfce4-modemlights-plugin], [$MODEM_LIGHTS_VERSION])
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

dnl check for standard header files
AC_HEADER_STDC
AC_CHECK_HEADERS(net/if.h)

dnl Check for i18n support
XDT_I18N([nl])

dnl Check for X11 installed
XDT_CHECK_LIBX11_REQUIRE
XDT_CHECK_LIBSM

dnl required
XDT_CHECK_PACKAGE([GMODULE], [gmodule-2.0], [2.4.0])
XDT_CHECK_PACKAGE([GTK], [gtk+-2.0], [2.4.0])
XDT_XFCE_PANEL_PLUGIN([XFCE4_PANEL], [4.2.0])

dnl check for debugging support
BM_DEBUG_SUPPORT

AC_OUTPUT([
Makefile
panel-plugin/Makefile
po/Makefile.in
pixmaps/Makefile
])
