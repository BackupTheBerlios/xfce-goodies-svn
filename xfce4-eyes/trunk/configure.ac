dnl configure.ac
dnl
dnl xfce4-eyes - eyes that spy on you
dnl
dnl

AC_INIT([configure])

AM_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE([xfce4-eyes], [4.3.0])

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
AC_CHECK_HEADERS([math.h])

dnl Check for math library
AC_CHECK_LIB([m], [sin])

dnl Check for i18n support
BM_I18N([xfce4-eyes], [ar az be bg bn_IN ca cs de en_GB es es_MX et eu
		       fa fi fr gu he hi hu it ja ko lt mr ms nl nb_NO
		       pa pl pt_BR pt_PT ro ru sv sk ta tr uk vi zh_CN
		       zh_TW ])

dnl Check for required packages
BM_DEPEND([LIBXFCEGUI4], [libxfcegui4-1.0], [4.2.0])

dnl configure the panel plugin
XFCE_PANEL_PLUGIN([XFCE4_PANEL], [4.2.0])

dnl Check for debugging support
BM_DEBUG_SUPPORT

AC_OUTPUT([
Makefile
xfce4-eyes.spec
po/Makefile.in
panel-plugin/Makefile
themes/Makefile
themes/Bizarre/Makefile
themes/Bloodshot/Makefile
themes/Default/Makefile
themes/Default-tiny/Makefile
themes/Horrid/Makefile
])
