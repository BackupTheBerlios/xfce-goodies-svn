#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "callbacks.h"
#include "main_gui.h"
#include "support.h"


void
on_new_clicked                         (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_load_clicked                        (GtkButton       *button,
                                        gpointer         user_data)
{

}



/*
void
on_drag_data                           (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data)
{

}*/


void
on_treeview1_drag_data_get             (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data)
{
printf("drag get\n");
}


gboolean
on_treeview1_drag_drop                 (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time,
                                        gpointer         user_data)
{
printf ("drag drop\n");
  return FALSE;
}

/*
gboolean
on_drag_motion                         (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time,
                                        gpointer         user_data)
{

  return FALSE;
}
*/
