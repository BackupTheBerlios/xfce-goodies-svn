plugindir = @XFCE4_PANEL_PLUGINSDIR@

plugin_LTLIBRARIES =							\
	libtaskbar.la

libtaskbar_la_LDFLAGS =						\
	-avoid-version							\
	-export-dynamic							\
	-module

libtaskbar_la_SOURCES =						\
	taskbar.c

libtaskbar_la_CFLAGS =						\
        -DPACKAGE_LOCALE_DIR=\"$(localedir)\"                           \
	@XFCE4_PANEL_CFLAGS@

libtaskbar_la_LIBADD = 						\
	@XFCE4_PANEL_LIBS@

EXTRA_DIST =
	
noinst_DATA =							
