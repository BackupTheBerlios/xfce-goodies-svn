## Process this file with automake to produce Makefile.in

INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-I/usr/local/include/libxml2	\
	@PACKAGE_CFLAGS@

bin_PROGRAMS = xffm_theme_maker

xffm_theme_maker_SOURCES = \
	main.c \
	support.c support.h \
	main_gui.c main_gui.h \
	callbacks.c callbacks.h

xffm_theme_maker_LDADD = @PACKAGE_LIBS@ -lxml2

