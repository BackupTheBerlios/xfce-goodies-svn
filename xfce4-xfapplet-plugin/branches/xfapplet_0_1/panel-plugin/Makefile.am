plugindir = $(libexecdir)/xfce4/panel-plugins
plugin_PROGRAMS = xfce4-xfapplet-plugin

xfce4_xfapplet_plugin_SOURCES = \
	xfapplet.h \
	xfapplet.c \
	chooser.c \
	trans.c

trans.c: ../AUTHORS
	echo -e '#include "xfapplet.h"\n\nXfAppletTranslators translators[] = {' > $@
	grep ':' $< | sort | sed -n 's/[ ]*\([a-zA-Z_]*\):[ ]*\(.*\) <\([^>]*\)>/{"\2", "\3", "\1",},/p' >> $@
	echo -e '{NULL,},};\n' >> $@

xfce4_xfapplet_plugin_CFLAGS = \
	@LIBPANELAPPLET_CFLAGS@ \
	@LIBXFCE4PANEL_CFLAGS@ \
	@LIBXFCE4UTIL_CFLAGS@ \
	@LIBXFCEGUI4_CFLAGS@ \
	-DDATADIR=\"$(datadir)\" \
	-DPKGDATADIR=\"$(pkgdatadir)\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	-DLIBXFCE4PANEL_LOCALE_DIR=\"@LIBXFCE4PANEL_LOCALE_DIR@\"

xfce4_xfapplet_plugin_LDFLAGS = \
	@LIBPANELAPPLET_LIBS@ \
	@LIBXFCE4PANEL_LIBS@ \
	@LIBXFCE4UTIL_LIBS@ \
	@LIBXFCEGUI4_LIBS@

uidir = $(pkgdatadir)/ui
ui_DATA = \
	XFCE_Panel_Popup.xml
EXTRA_DIST = $(ui_DATA)

# .desktop file
#
# Some automake trickery here. Because we cannot use $(libexecdir) in the
# automake stage, we'll use sed to get the full path into the .desktop file.
# We also need to let intltool merge the translated fields, so we add an
# additional level of indirection: a <name>.desktop.in.in file.
# 
desktop_in_in_files = xfapplet.desktop.in.in
desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in)

desktopdir = $(datadir)/xfce4/panel-plugins
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST += $(desktop_in_in_files)

DISTCLEANFILES = \
	$(desktop_DATA) $(desktop_in_files)

# get full path into .desktop file
%.desktop.in: %.desktop.in.in
	sed -e "s^@PLUGIN_PATH@^$(libexecdir)/xfce4/panel-plugins^" \
		$< > $@
