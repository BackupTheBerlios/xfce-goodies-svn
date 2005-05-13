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
#ifndef __XFCE_IMAGE_LIST_BOX_H__
#define __XFCE_IMAGE_LIST_BOX_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

/* properties:
	filename
	changed
*/

#define TYPE_XFCE_IMAGE_LIST_BOX	         (xfce_image_list_box_get_type ())
#define XFCE_IMAGE_LIST_BOX(obj)	         (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_XFCE_IMAGE_LIST_BOX, XfceImageListBox))
#define XFCE_IMAGE_LIST_BOX_CLASS(klass)	 (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_XFCE_IMAGE_LIST_BOX, XfceImageListBoxClass))
#define IS_XFCE_IMAGE_LIST_BOX(obj)	      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_XFCE_IMAGE_LIST_BOX))
#define IS_XFCE_IMAGE_LIST_BOX_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_XFCE_IMAGE_LIST_BOX))
#define XFCE_IMAGE_LIST_BOX_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_XFCE_IMAGE_LIST_BOX, XfceImageListBoxClass))

typedef struct _XfceImageListBox	    XfceImageListBox;
typedef struct _XfceImageListBoxClass       XfceImageListBoxClass;

typedef struct _XfceImageListBoxPrivate     XfceImageListBoxPrivate;

struct _XfceImageListBox
{
	GtkHBox parent;
	
	XfceImageListBoxPrivate *priv;
};

struct _XfceImageListBoxClass
{
	GtkHBoxClass parent_class;
	
	void (*selection_changed)(XfceImageListBox *lb);
};


GType xfce_image_list_box_get_type(void) G_GNUC_CONST;

XfceImageListBox* xfce_image_list_box_new(void);

gboolean xfce_image_list_box_load(XfceImageListBox *a, const gchar *filename);
gboolean xfce_image_list_box_save(XfceImageListBox *a, const gchar *filename);
void xfce_image_list_box_clear(XfceImageListBox *a);
void xfce_image_list_box_add_stuff(XfceImageListBox *a, const gchar *filename, int loopy); /* file or dir */
void xfce_image_list_box_add_file(XfceImageListBox *a, const gchar *filename);
void xfce_image_list_box_add_directory(XfceImageListBox *a, const gchar *dirname, int loopy);
gchar *xfce_image_list_box_get_selected_filename(XfceImageListBox *a);
gboolean xfce_image_list_box_find_filename(XfceImageListBox *a, const gchar *filename, GtkTreeIter *iter);
void xfce_image_list_box_select_filename(XfceImageListBox *a, const gchar *filename);
void xfce_image_list_box_remove (XfceImageListBox *a, GtkTreeIter *iter);

G_END_DECLS

#endif /* __XFCE_IMAGE_LIST_BOX_H__ */
