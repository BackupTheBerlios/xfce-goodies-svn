plugindir = @XFCE4_PANEL_PLUGINSDIR@

plugin_LTLIBRARIES =							\
	libluagraph.la

libluagraph_la_LDFLAGS =							\
	-llua50	\
	-llualib50 \
	-avoid-version							\
	-export-dynamic							\
	-module

libluagraph_la_SOURCES =							\
	luagraph.c

libluagraph_la_CFLAGS =							\
	@XFCE4_PANEL_CFLAGS@

libluagraph_la_LIBADD = 							\
	@XFCE4_PANEL_LIBS@
