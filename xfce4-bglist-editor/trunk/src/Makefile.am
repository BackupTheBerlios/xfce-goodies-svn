
bin_PROGRAMS = xfce4-bglist-editor

xfce4_bglist_editor_SOURCES = \
	aspect-scale.c aspect-scale.h \
	xfce-auto-scale-image.c xfce-auto-scale-image.h \
	xfce-cell-renderer-pixbuf-on-demand.c \
	xfce-cell-renderer-pixbuf-on-demand.h \
	xfce-image-file-chooser.c xfce-image-file-chooser.h \
	xfce-image-list-box.c xfce-image-list-box.h \
	xfce-image-list-dialog.c xfce-image-list-dialog.h \
	xfce-tree-helpers.c xfce-tree-helpers.h \
	xfce-background-get.c xfce-background-get.h \
	trace.h main.c icon.h

xfce4_bglist_editor_CFLAGS = \
	@GTK_CFLAGS@ \
	@XFCE4_GUI_CFLAGS@ \
	-DLOCALEDIR=\"$(localedir)\"
	
xfce4_bglist_editor_LDADD = \
	@XFCE4_GUI_LIBS@ \
	@GTK_LIBS@

