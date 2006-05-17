plugindir = $(libexecdir)/xfce4/panel-plugins
plugin_PROGRAMS = xfce4-battery-plugin

xfce4_battery_plugin_SOURCES =  \
	battery.h               \
	battery.c               \
	battery-hal.h           \
	battery-hal.c           \
	battery-dialogs.h       \
	battery-dialogs.c       \
        battery-overview.h      \
        battery-overview.c

xfce4_battery_plugin_CFLAGS =   \
	-I$(top_srcdir)         \
	@LIBXFCE4UTIL_CFLAGS@   \
	@LIBXFCE4PANEL_CFLAGS@  \
	@HAL_CFLAGS@            \
	@DBUS_CFLAGS@           \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"

xfce4_battery_plugin_LDFLAGS =  \
	@LIBXFCE4PANEL_LIBS@    \
	@LIBXFCE4UTIL_LIBS@     \
	@HAL_LIBS@              \
	@DBUS_LIBS@

desktop_in_in_files = battery.desktop.in.in
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
