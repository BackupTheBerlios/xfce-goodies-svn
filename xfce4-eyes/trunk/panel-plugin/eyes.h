#ifndef __EYES_H
#define __EYES_H
#include <gtk/gtk.h>

#define MAX_EYES 5

typedef struct
{
	char *theme;
} t_eyes_options;

typedef struct
{
	GtkWidget	*ebox;
/*	GtkWidget	*button;
*/
	/* Applet */
/*	GtkWidget *applet;*/
	GtkWidget   *align; /*fixed;*/
	GtkWidget   *hbox;
	GtkWidget   *eyes[MAX_EYES];
	guint        timeout_id;

	/* Theme */
	GdkPixbuf *eye_image;
	GdkPixbuf *pupil_image;
	gchar *theme_dir;
	gchar *theme_name;
	gchar *eye_filename;
	gchar *pupil_filename;
	gint num_eyes;
	gint eye_height;
	gint eye_width;
	gint pupil_height;
	gint pupil_width;
	gint wall_thickness;

	/* options dialog */
	GtkWidget *dialog;
/*	GtkWidget *revert_b;  revert button */
	GtkWidget *theme_om; /* theme combo box */
	
	t_eyes_options	options;
/*	t_eyes_options	revert;*/
} t_eyes;

#endif /* ndef __EYES_H */
