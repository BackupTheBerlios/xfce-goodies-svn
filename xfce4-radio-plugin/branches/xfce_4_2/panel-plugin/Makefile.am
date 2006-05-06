plugindir = @XFCE4_PANEL_PLUGINSDIR@

plugin_LTLIBRARIES =						\
	libradio.la

libradio_la_LDFLAGS =						\
	-avoid-version						\
	-export-dynamic						\
	-module

libradio_la_SOURCES =						\
	radio.c							\
	radio.h

libradio_la_CFLAGS =						\
        -DPACKAGE_LOCALE_DIR=\"$(localedir)\"                   \
	@XFCE4_PANEL_CFLAGS@

libradio_la_LIBADD = 						\
	@XFCE4_PANEL_LIBS@
