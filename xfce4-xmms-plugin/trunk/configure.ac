dnl configure.ac
dnl
dnl xfce4-xmms-plugin - XMMS Control plugin for xfce4 panel
dnl
dnl 2003 Patrick van Staveren <pvanstav@cs.wmich.edu>
dnl 2005 Kemal Ilgar Eroglu <kieroglu@math.washington.edu>


dnl version info
m4_define([xfce4_xmms_version], [0.4])

dnl init autoconf
AC_INIT([xfce4-xmms-plugin], [xfce4_xmms_version], [kieroglu@math.washington.edu])
AC_PREREQ([2.50])

dnl init automake
XFCE4_XMMS_VERSION=xfce4_xmms_version
AM_INIT_AUTOMAKE([xfce4-xmms-plugin], [$XFCE4_XMMS_VERSION])
AM_CONFIG_HEADER([config.h])
AM_MAINTAINER_MODE

dnl Check for basic stuff
AC_PROG_CC
AC_HEADER_STDC
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_INTLTOOL()

dnl check for standard header files
AC_HEADER_STDC

dnl Check for i18n support
XDT_I18N([])

dnl required
XDT_CHECK_PACKAGE([GTHREAD], [gthread-2.0], [2.4.0])
XDT_CHECK_PACKAGE([GTK], [gtk+-2.0], [2.4.0])
XDT_CHECK_PACKAGE([LIBXFCEGUI4], [libxfcegui4-1.0], [4.2.0])
XDT_CHECK_PACKAGE([LIBXFCE4PANEL], [libxfce4panel-1.0], [4.3.20])
XDT_CHECK_PACKAGE([LIBXFCE4UTIL], [libxfce4util-1.0], [4.3.0])

dnl Check for Pango >= 1.8.0
PKG_CHECK_MODULES(PANGO, pango >= 1.8.0,,exit)

dnl Check for Gtk 2.8. If yes, add Cairo flags
dnl CAIRO_CF=`pkg-config --cflags cairo`
dnl CAIRO_L=`pkg-config --libs cairo`
dnl PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 3.8.0, CAIRO_CFLAGS = CAIRO_CF, AC_MSG_RESULT())
dnl PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 3.8.0, CAIRO_LIBS = CAIRO_L, AC_MSG_RESULT())

dnl check for player
AC_MSG_CHECKING([what player to link with])
AC_ARG_WITH([player], [  --with-player=xmms/beep Specify player to link with [default=xmms]])
if test x"$with_player" == x"beep"; then
  AC_MSG_RESULT([Beep media player])
  BM_DEPEND([PLAYER], [beep], [0.9.6.0])

  PLAYER_CFLAGS="$PLAYER_CFLAGS -DPLAYER_COMMAND=\\\"beep-media-player\\\" -DPLAYER_BEEP"
else
  AC_MSG_RESULT([XMMS media player])

  AC_CHECK_PROGS([XMMS_CONFIG], [xmms-config], [no])
  if test x"$XMMS_CONFIG" == x"no"; then
    AC_MSG_ERROR([*** XMMS not found. Please install XMMS first. ***])
  fi

  AC_MSG_CHECKING([PLAYER_CFLAGS])
  PLAYER_CFLAGS=`$XMMS_CONFIG --cflags`
  AC_MSG_RESULT([$PLAYER_CFLAGS])

  AC_MSG_CHECKING([PLAYER_LIBS])
  PLAYER_LIBS=`$XMMS_CONFIG --libs`
  AC_MSG_RESULT([$PLAYER_LIBS])

  PLAYER_CFLAGS="$PLAYER_CFLAGS -DPLAYER_COMMAND=\\\"xmms\\\" -DPLAYER_XMMS"
fi
AC_SUBST([PLAYER_CFLAGS])
AC_SUBST([PLAYER_LIBS])
AC_SUBST([CAIRO_CFLAGS])
AC_SUBST([CAIRO_LIBS])

AC_OUTPUT([
Makefile
panel-plugin/Makefile
pixmaps/Makefile
po/Makefile.in
xfce4-xmms-plugin.spec
])
