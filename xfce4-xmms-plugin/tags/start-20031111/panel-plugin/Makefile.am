plugindir = @XFCE4_PANEL_PLUGINSDIR@

plugin_LTLIBRARIES =							\
	libxmms_plugin.la

libxmms_plugin_la_LDFLAGS =						\
	-avoid-version							\
	-export-dynamic							\
	-module								\
	`xmms-config --libs`

libxmms_plugin_la_SOURCES =						\
	xmms_plugin.c

libxmms_plugin_la_CFLAGS =						\
	@XFCE4_PANEL_CFLAGS@						\
        `xmms-config --cflags`						\
        -DDATA_DIR=\"/usr/local/share/xfce4/xmms-plugin\"

libxmms_plugin_la_LIBADD = 						\
	@XFCE4_PANEL_LIBS@

clean:	
	rm *~ -f
