plugindir = @XFCE4_PANEL_PLUGINSDIR@

plugin_LTLIBRARIES =							\
	libscreenshooter.la

libscreenshooter_la_LDFLAGS =							\
	-avoid-version							\
	-export-dynamic							\
	-module

libscreenshooter_la_SOURCES =							\
	shooter.c screenshot.c shooter_icon.h

libscreenshooter_la_CFLAGS =							\
	@XFCE4_PANEL_CFLAGS@

libscreenshooter_la_LIBADD = 							\
	@XFCE4_PANEL_LIBS@
