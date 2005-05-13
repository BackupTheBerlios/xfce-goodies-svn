/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Xfce Background List Editor
 * Copyright (C) 2004 Danny Milosavljevic <danny_milo@yahoo.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <libxfcegui4/xfce_scaled_image.h>
#include <libxfce4util/i18n.h>
#include <libxfcegui4/xfce-filechooser.h>

#include "xfce-image-list-dialog.h"
#include "xfce-image-list-box.h"
#include "trace.h"
#include "icon.h"

enum {
	LAST_SIGNAL
};

enum {
	LAST_PROP
};

struct _XfceImageListDialogPrivate
{
	XfceImageListBox *listbox;
	XfceScaledImage *image;
	GtkBox *image_box;
	GtkLabel *image_label;
	GtkPaned *paned;
	GtkButton *ok_button;
};

static GtkDialogClass *parent_class = NULL;

/* static guint XfceImageListDialog_signals[LAST_SIGNAL] = { 0 }; */

static void xfce_image_list_dialog_class_init(XfceImageListDialogClass *klass);
static void xfce_image_list_dialog_init(XfceImageListDialog *aXfceImageListDialog);
static void xfce_image_list_dialog_finalize(GObject *object);
static void xfce_image_list_dialog_dispose(GObject *object);

static void xfce_image_list_dialog_get_property(GObject *object, guint param_id, GValue *value, GParamSpec *pspec);
static void xfce_image_list_dialog_set_property(GObject *object, guint param_id, const GValue *value, GParamSpec *pspec);

GType
xfce_image_list_dialog_get_type(void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo our_info =
			{
				sizeof(XfceImageListDialogClass),
				NULL,	   /* base_init */
				NULL,	   /* base_finalize */
				(GClassInitFunc) xfce_image_list_dialog_class_init,
				NULL,	   /* class_finalize */
				NULL,	   /* class_data */
				sizeof(XfceImageListDialog),
				0,	      /* n_preallocs */
				(GInstanceInitFunc) xfce_image_list_dialog_init
			};
		
		type = g_type_register_static(GTK_TYPE_DIALOG,
					      "XfceImageListDialog",
					      &our_info,
					      0);
	}
	
	return type;
}
static void 
xfce_image_list_dialog_finalize(GObject *object)
{
	XfceImageListDialog *aXfceImageListDialog;

	g_return_if_fail(object != NULL);
	g_return_if_fail(IS_XFCE_IMAGE_LIST_DIALOG(object));

	aXfceImageListDialog = XFCE_IMAGE_LIST_DIALOG(object);

	if (G_OBJECT_CLASS(parent_class)->finalize)
	        (* G_OBJECT_CLASS(parent_class)->finalize)(object);

	g_free(aXfceImageListDialog->priv);
}
static void 
xfce_image_list_dialog_dispose(GObject *object)
{
	XfceImageListDialog *aXfceImageListDialog;

	g_return_if_fail(object != NULL);
	g_return_if_fail(IS_XFCE_IMAGE_LIST_DIALOG(object));

	aXfceImageListDialog = XFCE_IMAGE_LIST_DIALOG(object);

	if (G_OBJECT_CLASS(parent_class)->dispose)
	        (* G_OBJECT_CLASS(parent_class)->dispose)(object);
}
static void
xfce_image_list_dialog_get_property(GObject *object, guint param_id, GValue *value, GParamSpec *pspec)
{
	XfceImageListDialog *aXfceImageListDialog;        

	aXfceImageListDialog = XFCE_IMAGE_LIST_DIALOG(object);

	switch (param_id) {
	default:
	        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
	        break;
	}
}
static void
xfce_image_list_dialog_set_property(GObject *object, guint param_id, const GValue *value, GParamSpec *pspec)
{
	XfceImageListDialog *aXfceImageListDialog;        

	aXfceImageListDialog = XFCE_IMAGE_LIST_DIALOG(object);

	switch (param_id) {
	default:
	        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
	        break;
	}
}

static void
xfce_image_list_dialog_class_init(XfceImageListDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	parent_class = g_type_class_peek_parent(klass);
	
	object_class->finalize = xfce_image_list_dialog_finalize;
	object_class->dispose = xfce_image_list_dialog_dispose;
	object_class->get_property = xfce_image_list_dialog_get_property;
	object_class->set_property = xfce_image_list_dialog_set_property;

}

static void
xfce_image_list_dialog_selection_changed_cb(XfceImageListBox *listbox, XfceImageListDialog *a);

static void
xfce_image_list_dialog_notify_fname_changed_cb(XfceImageListBox *lb, GParamSpec *spec, XfceImageListDialog *a)
{
	gchar *fname;
	gchar *bn;
	gchar *title;
	TRACE ("fname changed, dialog");
	
	g_object_get (G_OBJECT (lb), spec->name, &fname, NULL);
	if (fname) {
		bn = g_path_get_basename (fname);
	} else bn = NULL;
	
	if (bn)
		title = g_strdup_printf (_("Background List Editor - %s"), bn);
	else
		title = g_strdup_printf (_("Background List Editor"));

	gtk_window_set_title (GTK_WINDOW (a), title);
	g_free (title);		
}

static void
xfce_image_list_dialog_notify_changed_cb(XfceImageListBox *lb, GParamSpec *spec, XfceImageListDialog *a)
{
	gboolean changed;
	TRACE ("xfce_image_list_dialog_notify_changed_cb");
	g_object_get (G_OBJECT (lb), spec->name, &changed, NULL);
	gtk_widget_set_sensitive (GTK_WIDGET (a->priv->ok_button), changed);
	if (changed)
		gtk_dialog_set_default_response (GTK_DIALOG (a), GTK_RESPONSE_OK);
	else
		gtk_dialog_set_default_response (GTK_DIALOG (a), GTK_RESPONSE_CANCEL);
}

static gchar *
xfce_image_list_dialog_list_save_as (XfceImageListDialog *a, gchar const *fname_default)
{
	/* TODO default ? */
	XfceFileChooser *fc;
	XfceFileFilter *filt1;
	XfceFileFilter *filt2;
	gint rc;
	gchar *fname;
	fname = NULL;
	TRACE ("list_save_as");
	
	fc = XFCE_FILE_CHOOSER (xfce_file_chooser_new ("Background List - Save as",
		GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (a))),
		XFCE_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_SAVE, GTK_RESPONSE_OK,
		NULL
		));

	xfce_file_chooser_set_select_multiple (fc, FALSE);
	xfce_file_chooser_set_current_name (fc, ".imglist");
	/*xfce_file_chooser_set_default_extension .imglist */

	filt1 = xfce_file_filter_new ();
	xfce_file_filter_set_name (filt1, _("Image File List"));
	xfce_file_filter_add_mime_type (filt1, "application/x-xfce-image-file-list");
	xfce_file_filter_add_pattern (filt1, "*.imglist");
	
	xfce_file_chooser_add_filter (fc, filt1);

	filt2 = xfce_file_filter_new ();
	xfce_file_filter_set_name (filt2, _("All Files"));
	xfce_file_chooser_add_filter (fc, filt2);

	rc = gtk_dialog_run (GTK_DIALOG (fc));
	if (rc == GTK_RESPONSE_OK) {
		fname = xfce_file_chooser_get_filename (fc);
		gtk_widget_destroy (GTK_WIDGET (fc));
		
		return fname;
	}
	
	return NULL;
}

void
xfce_image_list_dialog_ok_button_clicked_cb(GtkWidget *button, XfceImageListDialog *a)
{
	gchar *fname;
	fname = NULL;
	g_object_get (G_OBJECT (a->priv->listbox), "filename", &fname, NULL);
	if (!fname || !fname[0]) {
		fname = xfce_image_list_dialog_list_save_as (a, NULL);
		g_object_set (G_OBJECT (a->priv->listbox), "filename", fname, NULL);
	}
	if(fname && fname[0]) {
		xfce_image_list_dialog_save_list (a, fname);
		g_free (fname);
	}
}


static void 
xfce_image_list_dialog_init(XfceImageListDialog *aXfceImageListDialog)
{
	XfceImageListDialogPrivate *priv;
	GdkPixbuf*	pixbuf;

	pixbuf = gdk_pixbuf_new_from_inline (-1, my_pixbuf, FALSE, NULL);
	if (pixbuf) {
		gtk_window_set_icon (GTK_WINDOW (aXfceImageListDialog), pixbuf);
		g_object_unref (G_OBJECT (pixbuf));
	}

	priv = g_new0(XfceImageListDialogPrivate, 1);

	aXfceImageListDialog->priv = priv;
	
	priv->listbox = xfce_image_list_box_new ();

	priv->image_label = GTK_LABEL (gtk_label_new (""));
	gtk_widget_show (GTK_WIDGET (priv->image_label));

	priv->image = XFCE_SCALED_IMAGE (xfce_scaled_image_new ());
	gtk_widget_set_size_request (GTK_WIDGET (priv->image), 400, 400);

	priv->image_box = GTK_BOX (gtk_vbox_new (FALSE, 7));
	gtk_container_set_border_width (GTK_CONTAINER (priv->image_box), 5);
	gtk_box_pack_start (GTK_BOX (priv->image_box), GTK_WIDGET (priv->image_label), FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (priv->image_box), GTK_WIDGET (priv->image), FALSE, FALSE, 0);
	gtk_widget_show_all (GTK_WIDGET (priv->image_box));

	gtk_widget_show (GTK_WIDGET (priv->listbox));
	
	priv->paned = GTK_PANED (gtk_hpaned_new ());
	gtk_paned_pack1 (priv->paned, GTK_WIDGET (priv->listbox), TRUE, TRUE);
	gtk_paned_pack2 (priv->paned, GTK_WIDGET (priv->image_box), TRUE, TRUE);
	
	gtk_widget_show (GTK_WIDGET (priv->paned));

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (aXfceImageListDialog)->vbox), GTK_WIDGET (priv->paned), TRUE, TRUE, 0);	

	gtk_dialog_add_button (GTK_DIALOG (aXfceImageListDialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	priv->ok_button = GTK_BUTTON (gtk_dialog_add_button (GTK_DIALOG (aXfceImageListDialog), GTK_STOCK_OK, GTK_RESPONSE_OK));
	gtk_dialog_set_default_response (GTK_DIALOG (aXfceImageListDialog), GTK_RESPONSE_CANCEL);
	gtk_widget_set_sensitive (GTK_WIDGET (priv->ok_button), FALSE);
                 
	g_signal_connect (G_OBJECT (priv->listbox), "selection-changed", G_CALLBACK (xfce_image_list_dialog_selection_changed_cb), aXfceImageListDialog);
	g_signal_connect (G_OBJECT (priv->listbox), "notify::changed", G_CALLBACK (xfce_image_list_dialog_notify_changed_cb), aXfceImageListDialog);
	g_signal_connect (G_OBJECT (priv->listbox), "notify::filename", G_CALLBACK (xfce_image_list_dialog_notify_fname_changed_cb), aXfceImageListDialog);
	
	g_signal_connect (G_OBJECT (priv->ok_button), "clicked", G_CALLBACK (xfce_image_list_dialog_ok_button_clicked_cb), aXfceImageListDialog);
}


static void
xfce_image_list_dialog_selection_changed_cb(XfceImageListBox *listbox, XfceImageListDialog *a)
{
	gchar *filename;
	XfceImageListDialogPrivate *priv;
	GdkPixbuf *pb;
	
	priv = a->priv;

	filename = xfce_image_list_box_get_selected_filename (priv->listbox);
	if (filename) {
		pb = gdk_pixbuf_new_from_file (filename, NULL);
		xfce_scaled_image_set_from_pixbuf (priv->image, pb);
		if (pb)
			g_object_unref (G_OBJECT (pb));

		g_free (filename);
	}
}

XfceImageListDialog*
xfce_image_list_dialog_new(void)
{
	XfceImageListDialog *aXfceImageListDialog;
	XfceImageListDialogPrivate *priv;

	aXfceImageListDialog = g_object_new(xfce_image_list_dialog_get_type(), NULL);
	
	priv = aXfceImageListDialog->priv;

	return aXfceImageListDialog;
}

void 
xfce_image_list_dialog_clear_list(XfceImageListDialog *a)
{
	XfceImageListDialogPrivate *priv;
	priv = a->priv;

	xfce_image_list_box_clear (priv->listbox);
}

gboolean 
xfce_image_list_dialog_load_list(XfceImageListDialog *a, const gchar *filename)
{
	XfceImageListDialogPrivate *priv;
	priv = a->priv;
	
	return xfce_image_list_box_load (priv->listbox, filename);	
}

gboolean 
xfce_image_list_dialog_save_list(XfceImageListDialog *a, const gchar *filename)
{
	XfceImageListDialogPrivate *priv;
	priv = a->priv;

	return xfce_image_list_box_save (priv->listbox, filename);
}

void
xfce_image_list_dialog_select_filename(XfceImageListDialog *a, const gchar *filename)
{
	XfceImageListDialogPrivate *priv;
	priv = a->priv;

	return xfce_image_list_box_select_filename (priv->listbox, filename);
}

