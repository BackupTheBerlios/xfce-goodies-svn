plugindir = @XFCE4_PANEL_PLUGINSDIR@

plugin_LTLIBRARIES =							\
	libxmms_plugin.la

libxmms_plugin_la_LDFLAGS =						\
	-avoid-version							\
	-module

libxmms_plugin_la_SOURCES =						\
	xmms_plugin.c

libxmms_plugin_la_CFLAGS =						\
	@XFCE4_PANEL_CFLAGS@						\
	@PLAYER_CFLAGS@							\
        -DDATA_DIR=\"$(datadir)/xfce4/xmms-plugin\"

libxmms_plugin_la_LIBADD = 						\
	@XFCE4_PANEL_LIBS@						\
	@PLAYER_LIBS@
