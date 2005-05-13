#include <stdio.h>
#include <gtk/gtk.h>
#include "xfce-image-list-dialog.h"
#include "xfce-background-get.h"

static void
usage()
{
	fprintf(stderr, "Usage: xfce4-bglist-editor [<file.imglist>]\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	XfceImageListDialog *dlg;
	gint i;
	gchar *current;
	gtk_init (&argc, &argv);

	dlg = xfce_image_list_dialog_new ();

	xfce_image_list_dialog_clear_list (dlg);
		
	if (argc > 1) {
		i = 1;
		if (g_str_equal(argv[i], "--help") || g_str_equal(argv[i], "-h"))
			usage();
			
		if (g_str_equal(argv[i], "--"))
			i++;
		
		if (argv[i])
			xfce_image_list_dialog_load_list (dlg, argv[i]);
	}

	current = xfce_background_get_current_image ();
	if (current) {
		xfce_image_list_dialog_select_filename (dlg, current);
		g_free (current);
	}
	
	gtk_dialog_run (GTK_DIALOG (dlg));
	
	/*gtk_main ();*/
	return 0;
}
