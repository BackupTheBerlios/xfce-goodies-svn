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
#ifndef __XFCE_IMAGE_LIST_DIALOG_H__
#define __XFCE_IMAGE_LIST_DIALOG_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TYPE_XFCE_IMAGE_LIST_DIALOG	         (xfce_image_list_dialog_get_type ())
#define XFCE_IMAGE_LIST_DIALOG(obj)	         (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_XFCE_IMAGE_LIST_DIALOG, XfceImageListDialog))
#define XFCE_IMAGE_LIST_DIALOG_CLASS(klass)	 (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_XFCE_IMAGE_LIST_DIALOG, XfceImageListDialogClass))
#define IS_XFCE_IMAGE_LIST_DIALOG(obj)	      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_XFCE_IMAGE_LIST_DIALOG))
#define IS_XFCE_IMAGE_LIST_DIALOG_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_XFCE_IMAGE_LIST_DIALOG))
#define XFCE_IMAGE_LIST_DIALOG_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_XFCE_IMAGE_LIST_DIALOG, XfceImageListDialogClass))

typedef struct _XfceImageListDialog	    XfceImageListDialog;
typedef struct _XfceImageListDialogClass       XfceImageListDialogClass;

typedef struct _XfceImageListDialogPrivate     XfceImageListDialogPrivate;

struct _XfceImageListDialog
{
	GtkDialog parent;
	
	XfceImageListDialogPrivate *priv;
};

struct _XfceImageListDialogClass
{
	GtkDialogClass parent_class;
};


GType xfce_image_list_dialog_get_type(void) G_GNUC_CONST;

XfceImageListDialog* xfce_image_list_dialog_new(void);

void xfce_image_list_dialog_clear_list(XfceImageListDialog *a);
gboolean xfce_image_list_dialog_load_list(XfceImageListDialog *a, const gchar *filename);
gboolean xfce_image_list_dialog_save_list(XfceImageListDialog *a, const gchar *filename);
void xfce_image_list_dialog_select_filename(XfceImageListDialog *a, const gchar *filename);

G_END_DECLS

#endif /* __XFCE_IMAGE_LIST_DIALOG_H__ */
