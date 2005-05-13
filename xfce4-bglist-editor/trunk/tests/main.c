#include <gtk/gtk.h>
#include "xfce-get-background.h"

int main(int argc, char *argv[])
{
	gchar *bgimage;
	gtk_init (&argc, &argv);

	bgimage = xfce_get_current_background_image ();
	if (bgimage) {
		printf("%s\n", bgimage);
		g_free (bgimage);
	}
	return 0;
}
