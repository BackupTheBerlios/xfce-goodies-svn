dnl configure.ac
dnl
dnl xfce4-lua-graph-plugin - Generic Lua-Driven Graph for xfce4-panel
dnl
dnl Copyright © 2005      A. Gordon
dnl
dnl Heavily based on Megahertz xfce plugin (version 0.1), by
dnl Copyright © 2005 Wit Wilinski <madman@linux.bydg.org>
dnl

AC_INIT
AC_CONFIG_SRCDIR([panel-plugin/luagraph.c])

AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([xfce4-lua-graph-plugin], [0.1])

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

dnl check for the Lua library
AC_CHECK_LIB([lua50],[lua_open],,[AC_MSG_ERROR([Please install the lua library development suite (lua50 library not found])])
AC_CHECK_LIB([lualib50],[luaopen_base],,[AC_MSG_ERROR([Please install the lualib library development suite (lualib50 library not found])])

dnl configure the panel plugin
XFCE_PANEL_PLUGIN([XFCE4_PANEL], [4.2.0])

dnl Check for debugging support
BM_DEBUG_SUPPORT

AC_CONFIG_FILES([
Makefile
panel-plugin/Makefile
])
AC_OUTPUT
