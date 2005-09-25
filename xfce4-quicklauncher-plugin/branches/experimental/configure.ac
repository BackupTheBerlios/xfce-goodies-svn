dnl 
dnl 	configure.ac
dnl	quicklauncher plugin 
dnl	2005 Masse Nicolas (masse_nicolas@yahoo.fr)
dnl

AC_INIT(panel-plugin/main.c)
AC_CONFIG_SRCDIR([configure])

AC_CONFIG_HEADER(config.h)

AM_INIT_AUTOMAKE([xfce4-quicklauncher-plugin], [1.9.0])

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

dnl configure the panel plugin
XDT_CHECK_PACKAGE([LIBXFCE4PANEL], [libxfce4panel-1.0], [4.3.20])

dnl Check for debugging support
XDT_FEATURE_DEBUG()

dnl For internationalisation
BM_I18N([xfce4-quicklauncher-plugin], [fr])

AC_OUTPUT([  
panel-plugin/Makefile
po/Makefile.in 
Makefile
])
