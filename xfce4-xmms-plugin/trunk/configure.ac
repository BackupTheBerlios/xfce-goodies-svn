dnl configure.ac
dnl
dnl xfce4-xmms-plugin - XMMS Control plugin for xfce4 panel
dnl
dnl 2003 Patrick van Staveren <pvanstav@cs.wmich.edu>
dnl

AC_INIT([panel-plugin/xmms_plugin.c])

AM_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE([xfce4-xmms-plugin], [0.1.2])

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
XFCE_PANEL_PLUGIN([XFCE4_PANEL], [3.99.1])

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

dnl Check for debugging support
BM_DEBUG_SUPPORT

AC_OUTPUT([
Makefile
panel-plugin/Makefile
pixmaps/Makefile
])
