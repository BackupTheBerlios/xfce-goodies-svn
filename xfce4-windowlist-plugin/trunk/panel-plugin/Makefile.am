plugindir = @XFCE4_PANEL_PLUGINSDIR@

plugin_LTLIBRARIES =							\
	libwindowlist.la

libwindowlist_la_LDFLAGS =						\
	-avoid-version							\
	-export-dynamic							\
	-module

libwindowlist_la_SOURCES =						\
	windowlist.c

libwindowlist_la_CFLAGS =						\
        -DPACKAGE_LOCALE_DIR=\"$(localedir)\"                           \
	@XFCE4_PANEL_CFLAGS@

libwindowlist_la_LIBADD = 						\
	@XFCE4_PANEL_LIBS@

EXTRA_DIST =
	
noinst_DATA =							
