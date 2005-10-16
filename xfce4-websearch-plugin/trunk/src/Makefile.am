plugindir = $(libdir)/xfce4/panel-plugins

plugin_LTLIBRARIES = \
	libwebsearch.la

libwebsearch_la_SOURCES = \
	websearch.c

libwebsearch_la_CFLAGS = \
	@LIBXFCE4PANEL_CFLAGS@ \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	-Wall

libwebsearch_la_LDFLAGS = \
	-avoid-version  \
	-export-dynamic \
	-module

libwebsearch_la_LIBADD = \
	@LIBXFCE4PANEL_LIBS@

# get full path into .desktop file
%.desktop.in: %.desktop.in.in
	sed -e "s^@INTERNAL_PLUGIN_PATH@^$(libdir)/xfce4/panel-plugins^"  < $< > $@

desktop_in_in_files = websearch.desktop.in.in
desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in)

desktopdir = $(datadir)/xfce4/panel-plugins
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	$(desktop_in_in_files)

DISTCLEANFILES = \
	$(desktop_DATA) $(desktop_in_files)
