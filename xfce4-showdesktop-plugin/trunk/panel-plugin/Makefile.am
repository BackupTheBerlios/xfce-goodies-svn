plugindir = @XFCE4_PANEL_PLUGINSDIR@

plugin_LTLIBRARIES =							\
	libshowdesktop.la

libshowdesktop_la_LDFLAGS =						\
	-avoid-version							\
	-export-dynamic							\
	-module

libshowdesktop_la_SOURCES =						\
	showdesktop.c

libshowdesktop_la_CFLAGS =						\
        -DPACKAGE_LOCALE_DIR=\"$(localedir)\"                           \
	@XFCE4_PANEL_CFLAGS@

libshowdesktop_la_LIBADD = 						\
	@XFCE4_PANEL_LIBS@
