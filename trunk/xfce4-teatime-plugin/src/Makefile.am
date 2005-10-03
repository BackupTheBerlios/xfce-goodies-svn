## Process this file with automake to produce Makefile.in

INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@PACKAGE_CFLAGS@

bin_PROGRAMS = teatime-plugin

teatime_plugin_SOURCES = \
	main.c \
	support.c support.h \
	interface.c interface.h \
	callbacks.c callbacks.h

teatime_plugin_LDADD = @PACKAGE_LIBS@ $(INTLLIBS)

