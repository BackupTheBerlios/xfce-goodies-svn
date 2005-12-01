plugindir = $(libexecdir)/xfce4/panel-plugins
plugin_PROGRAMS = xfce4-xmms-plugin

xfce4_xmms_plugin_SOURCES = 						\
		xmms_plugin.c

xfce4_xmms_plugin_CFLAGS =						\
	-I$(top_srcdir) 						\
	@LIBXFCEGUI4_CFLAGS@ 						\
	@LIBXFCE4UTIL_CFLAGS@ 						\
	@LIBXFCE4PANEL_CFLAGS@ 						\
	-DLOCALEDIR=\"$(localedir)\" 					\
	-DDATA_DIR=\"$(datadir)/xfce4/xfce4-xmms-plugin\"		\
	@CAIRO_CFLAGS@							\
	@PLAYER_CFLAGS@

xfce4_xmms_plugin_LDFLAGS =						\
	@LIBXFCE4PANEL_LIBS@ 						\
	@LIBXFCE4UTIL_LIBS@ 						\
	@LIBXFCEGUI4_LIBS@
	
xfce4_xmms_plugin_LDADD = 						\
	@PLAYER_LIBS@							\
	@CAIRO_LIBS@

desktop_in_in_files = xfce4-xmms-plugin.desktop.in.in
desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in)
	
desktopdir = $(datadir)/xfce4/panel-plugins
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = 				\
	$(desktop_in_in_files)

DISTCLEANFILES =			\
	$(desktop_DATA) $(desktop_in_files)
	
# get full path into .desktop file
%.desktop.in: %.desktop.in.in
	sed -e "s^@EXTERNAL_PLUGIN_PATH@^$(libexecdir)/xfce4/panel-plugins^" \
		$< > $@
