#include <gtk/gtk.h>
#include "xfceautoscaleimage.h"


int main(int argc, char *argv[])
{
	GtkWidget *w;
	XfceAutoScaleImage *image;
	gtk_init (&argc, &argv);
	
	w = gtk_window_new (GTK_WINDOW_TOPLEVEL);

	image = xfce_auto_scale_image_new ();
	xfce_auto_scale_image_set_from_file (image, "a.png");
	gtk_container_add (GTK_CONTAINER (w), GTK_WIDGET (image));
	gtk_widget_show_all (GTK_WIDGET (w));
	
	gtk_main ();
}
