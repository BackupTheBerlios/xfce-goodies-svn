plugindir = @XFCE4_PANEL_PLUGINSDIR@
plugin_LTLIBRARIES = libmodemlights.la

libmodemlights_la_LDFLAGS = \
	-avoid-version \
	-export-dynamic \
	-module

if HAVE_CYGWIN
  libmodemlights_la_LDFLAGS += \
	-no-undefined \
	-export-symbols $(datadir)/xfce4/devel/panel.def
endif

libmodemlights_la_SOURCES = \
	modemlights-plugin.c

libmodemlights_la_CFLAGS = \
	@XFCE4_PANEL_CFLAGS@ \
	-DDATADIR=\"$(datadir)\" \
	-DLOCALEDIR=\"$(localedir)\"

libmodemlights_la_LIBADD = \
	@XFCE4_PANEL_LIBS@
