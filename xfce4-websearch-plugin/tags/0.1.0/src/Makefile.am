plugindir = @XFCE4_PANEL_PLUGINSDIR@

plugin_LTLIBRARIES = libwebsearch.la

libwebsearch_la_SOURCES = websearch.c

libwebsearch_la_CFLAGS = \
	@XFCE4_PANEL_CFLAGS@ \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	-Wall

libwebsearch_la_LDFLAGS = \
	-avoid-version  \
	-export-dynamic \
	-module

libwebsearch_la_LIBADD = @XFCE4_PANEL_LIBS@
