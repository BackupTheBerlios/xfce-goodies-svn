plugindir = @XFCE4_PANEL_PLUGINSDIR@

plugin_LTLIBRARIES =							\
	libnetload.la

libnetload_la_LDFLAGS =							\
	-avoid-version							\
	-export-dynamic							\
	-module

libnetload_la_SOURCES =							\
	netload.c							\
	net.h								\
	net_@OS@.c

libnetload_la_CFLAGS =							\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	@XFCE4_PANEL_CFLAGS@

libnetload_la_LIBADD = 							\
	@XFCE4_PANEL_LIBS@
