plugindir = @XFCE4_PANEL_PLUGINSDIR@

plugin_LTLIBRARIES =							\
	libmegahertz.la

libmegahertz_la_LDFLAGS =							\
	-avoid-version							\
	-export-dynamic							\
	-module

libmegahertz_la_SOURCES =							\
	megahertz.c

libmegahertz_la_CFLAGS =							\
	@XFCE4_PANEL_CFLAGS@

libmegahertz_la_LIBADD = 							\
	@XFCE4_PANEL_LIBS@
