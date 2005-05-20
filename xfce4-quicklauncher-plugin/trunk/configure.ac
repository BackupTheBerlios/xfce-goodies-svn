dnl 
dnl 	configure.ac
dnl	quicklauncher plugin 
dnl	2005 Masse Nicolas (masse_nicolas@yahoo.fr)
dnl

AC_INIT(panel-plugin/main.c)
AC_CONFIG_SRCDIR([configure])

AC_CONFIG_HEADER(config.h)

AM_INIT_AUTOMAKE([xfce4-quicklauncher-plugin], [0.8.0])

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
XFCE_PANEL_PLUGIN([XFCE4_PANEL], [4.1.9.0])

dnl Check for debugging support
BM_DEBUG_SUPPORT

dnl For internationalisation
ALL_LINGUAS="fr"
AM_GNU_GETTEXT
AC_DEFINE_UNQUOTED(LOCALEDIR, "${prefix}/${DATADIRNAME}/locale", [Name of gettext locale directory])


AC_OUTPUT([  
panel-plugin/Makefile
intl/Makefile 
po/Makefile.in 
m4/Makefile 
Makefile
])
