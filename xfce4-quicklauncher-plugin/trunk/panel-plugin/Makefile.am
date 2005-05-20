plugindir = @XFCE4_PANEL_PLUGINSDIR@

plugin_LTLIBRARIES =							\
	libquicklauncher.la

libquicklauncher_la_LDFLAGS =							\
	-avoid-version							\
	-export-dynamic							\
	-module

libquicklauncher_la_SOURCES =							\
	callbacks.c							\
	main.c						
	

libquicklauncher_la_CFLAGS =							\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	@XFCE4_PANEL_CFLAGS@

libquicklauncher_la_LIBADD = 							\
	@XFCE4_PANEL_LIBS@
