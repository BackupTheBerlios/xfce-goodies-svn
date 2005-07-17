plugindir = $(libdir)/xfce4/panel-plugins

plugin_LTLIBRARIES =							\
	libeyes.la

libeyes_la_LDFLAGS =							\
	-avoid-version							\
	-export-dynamic							\
	-module

libeyes_la_SOURCES =							\
	eyes.c themes.h eyes.h themes.c

libeyes_la_CFLAGS =							\
	-DTHEMESDIR=\"$(datadir)/xfce4/eyes/themes\"			\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	@XFCE4_PANEL_CFLAGS@

libeyes_la_LIBADD = 							\
	@XFCE4_PANEL_LIBS@

if HAVE_CYGWIN
libeyes_la_LDFLAGS +=							\
	-no-undefined							\
	-export-symbols $(datadir)/xfce4/devel/panel.def
endif
