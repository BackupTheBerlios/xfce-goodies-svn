plugindir = @XFCE4_PANEL_PLUGINSDIR@

plugin_LTLIBRARIES =							\
	libminicmd.la

libminicmd_la_LDFLAGS =							\
	-avoid-version							\
	-export-dynamic							\
	-module

libminicmd_la_SOURCES =							\
	command.c

libminicmd_la_CFLAGS =							\
	@XFCE4_PANEL_CFLAGS@                                            \
        @LIB_XFCE4_UTIL_CFLAGS@

libminicmd_la_LIBADD = 							\
	@XFCE4_PANEL_LIBS@                                              \
        @LIB_XFCE4_UTIL_LIBS@
