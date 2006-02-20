plugindir = $(libdir)/xfce4/panel-plugins

desktopdatadir = @PANEL_DESKTOPDATADIR@
desktopdata_DATA = xfce4-eyes.desktop

# .desktop file
#
# Some automake trickery here. Because we cannot use $(libexecdir) in the
# automake stage, we'll use sed to get the full path into the .desktop file.
# We also need to let intltool merge the translated fields, so we add an
# additional level of indirection: a <name>.desktop.in.in file.
# 
desktop_in_in_files = xfce4-eyes.desktop.in.in
desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in)
	
desktopdir = $(datadir)/xfce4/panel-plugins
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = $(desktop_in_in_files)
DISTCLEANFILES =                                                        \
        $(desktop_DATA) $(desktop_in_files)


# get full path into .desktop file
%.desktop.in: %.desktop.in.in
	sed -e "s^@INTERNAL_PLUGIN_PATH@^$(libdir)/xfce4/panel-plugins^" \
		$< > $@
        
plugin_LTLIBRARIES =							\
	libeyes.la

libeyes_la_LDFLAGS =							\
	-avoid-version							\
	-export-dynamic							\
	-module

libeyes_la_SOURCES =							\
	eyes.c themes.h eyes.h themes.c

libeyes_la_CFLAGS =							\
	-DTHEMESDIR=\"$(datadir)/xfce4/eyes/themes\"			\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	@XFCE4_PANEL_CFLAGS@

libeyes_la_LIBADD = 							\
	@XFCE4_PANEL_LIBS@

if HAVE_CYGWIN
libeyes_la_LDFLAGS +=							\
	-no-undefined							\
	-export-symbols $(datadir)/xfce4/devel/panel.def
endif
