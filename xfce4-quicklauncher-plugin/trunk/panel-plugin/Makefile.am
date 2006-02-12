
plugindir = $(libdir)/xfce4/panel-plugins

plugin_LTLIBRARIES =							\
	libquicklauncher.la

libquicklauncher_la_LDFLAGS =							\
	-avoid-version							\
	-module

libquicklauncher_la_SOURCES =							\
	callbacks.c							\
	main.c						
	

libquicklauncher_la_CFLAGS =							\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	@LIBXFCE4PANEL_CFLAGS@

libquicklauncher_la_LIBADD = 							\
	@LIBXFCE4PANEL_LIBS@
	
# .desktop file
#
# Some automake trickery here. Because we cannot use $(libexecdir) in the
# automake stage, we'll use sed to get the full path into the .desktop file.
# We also need to let intltool merge the translated fields, so we add an
# additional level of indirection: a <name>.desktop.in.in file.

desktop_in_in_files = quicklauncher.desktop.in.in
desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in)
	
desktopdir = $(datadir)/xfce4/panel-plugins
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = 								\
	$(desktop_in_in_files)

DISTCLEANFILES =							\
	$(desktop_DATA) $(desktop_in_files)
	
# get full path into .desktop file
%.desktop.in: %.desktop.in.in
	sed -e "s^@INTERNAL_PLUGIN_PATH@^$(libdir)/xfce4/panel-plugins^" \
		$< > $@
