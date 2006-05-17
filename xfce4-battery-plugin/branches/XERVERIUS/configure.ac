dnl configure.ac
dnl
dnl xfce4-battery-plugin - Battery Monitor for the Xfce Panel
dnl
dnl 2005 Nick Schermer <nick@xfce.org>
dnl

dnl version info
m4_define([battery_version], [?.?.?])

dnl init autoconf
AC_INIT([xfce4-battery-plugin], [battery_version], [nick@xfce.org])
AC_PREREQ([2.50])

dnl init automake
BATTERY_VERSION=battery_version
AM_INIT_AUTOMAKE([xfce4-battery-plugin], [$BATTERY_VERSION])
AM_CONFIG_HEADER([config.h])
AM_MAINTAINER_MODE

dnl check for basic programs
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_INTLTOOL()

dnl add configure option
AC_ARG_ENABLE([dummies],
              [AC_HELP_STRING([--enable-dummies],
              [This option will add 3 (static) dummy batteries for better testing [default=no]])]
              ,,
              [enable_dummies])
              
if test "$enable_dummies" = yes ; then
    AC_DEFINE(DUMMIES,,[Use Dummies])
fi

dnl check for standard header files
AC_HEADER_STDC

dnl Check for i18n support
XDT_I18N([
	  ])

dnl Check for X11 installed
XDT_CHECK_LIBX11_REQUIRE
XDT_CHECK_LIBSM

dnl required
XDT_CHECK_PACKAGE([GTHREAD], [gthread-2.0], [2.6.0])
XDT_CHECK_PACKAGE([GTK], [gtk+-2.0], [2.6.0])
XDT_CHECK_PACKAGE([LIBXFCE4UTIL], [libxfce4util-1.0], [4.3.90])
XDT_CHECK_PACKAGE([LIBXFCEGUI4], [libxfcegui4-1.0], [4.3.90])
XDT_CHECK_PACKAGE([LIBXFCE4PANEL], [libxfce4panel-1.0], [4.3.90])
XDT_CHECK_PACKAGE([HAL], [hal], [0.5.6])
XDT_CHECK_PACKAGE([DBUS], [dbus-glib-1], [0.34])

AC_MSG_CHECKING([whether to use new header dialog])
if $PKG_CONFIG --atleast-version 4.3.90.2 libxfcegui4-1.0 >/dev/null 2>&1; then
  AC_DEFINE([USE_NEW_DIALOG], [1], [Define to use new header dialog])
  AC_MSG_RESULT([yes])
else
  AC_MSG_RESULT([no])
fi

dnl check for debugging support
XDT_FEATURE_DEBUG()

AC_OUTPUT([
Makefile
panel-plugin/Makefile
icons/Makefile
icons/16x16/Makefile
icons/22x22/Makefile
icons/scalable/Makefile
po/Makefile.in
])

dnl Print configuration
echo
echo "Build Configuration:"
echo
if test "$enable_dummies" = yes ; then
echo "* Dummy Batteries:           yes"
else
echo "* Dummy Batteries:           no"
fi
echo
