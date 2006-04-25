plugindir = $(libdir)/xfce4/panel-plugins/

plugin_LTLIBRARIES =							\
	libminicmd.la

libminicmd_la_LDFLAGS =							\
	-avoid-version							\
	-export-dynamic							\
	-module

libminicmd_la_SOURCES =							\
	command.c

libminicmd_la_CFLAGS =							\
	@LIBXFCE4PANEL_CFLAGS@

libminicmd_la_LIBADD = 							\
	@LIBXFCE4PANEL_LIBS@

%.desktop.in: %.desktop.in.in
	sed -e "s^@INTERNAL_PLUGIN_PATH@^$(libdir)/xfce4/panel-plugins^"  < $< > $@

desktop_in_in_files = minicmd.desktop.in.in
desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in)

desktopdir = $(datadir)/xfce4/panel-plugins
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST =                                                           \
	$(desktop_in_in_files)

DISTCLEANFILES =                                                       \
	$(desktop_DATA) $(desktop_in_files)
		
