#include <gtk/gtk.h>

enum {
	PIXBUF_COLUMN,
	GROUP_COLUMN,
	NAME_COLUMN,
	ICON_COLUMN,
	TREE_COLUMNS
};

void
on_new_clicked                         (GtkButton       *button,
                                        gpointer         user_data);

void
on_load_clicked                        (GtkButton       *button,
                                        gpointer         user_data);

void
on_save_clicked                        (GtkButton       *button,
                                        gpointer         user_data);

void
on_quit_clicked                        (GtkButton       *button,
                                        gpointer         user_data);

void
on_drag_data                           (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data);

void
on_treeview1_drag_data_get             (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data);

gboolean
on_treeview1_drag_drop                 (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time,
                                        gpointer         user_data);

gboolean
on_drag_motion                         (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time,
                                        gpointer         user_data);
