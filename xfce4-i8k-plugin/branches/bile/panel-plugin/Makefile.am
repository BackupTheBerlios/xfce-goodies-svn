plugindir = @XFCE4_PANEL_PLUGINSDIR@

plugin_LTLIBRARIES =							\
	libi8k.la

libi8k_la_LDFLAGS =							\
	-avoid-version							\
	-export-dynamic							\
	-module

libi8k_la_SOURCES =							\
	i8k.c

libi8k_la_CFLAGS =							\
	@XFCE4_PANEL_CFLAGS@

libi8k_la_LIBADD = 							\
	@XFCE4_PANEL_LIBS@
