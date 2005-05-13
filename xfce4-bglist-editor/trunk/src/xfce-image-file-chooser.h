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
#ifndef __XFCE_IMAGE_FILE_CHOOSER_H__
#define __XFCE_IMAGE_FILE_CHOOSER_H__

#include <gtk/gtk.h>
#include <libxfcegui4/libxfcegui4.h>

G_BEGIN_DECLS

#define TYPE_XFCE_IMAGE_FILE_CHOOSER	         (xfce_image_file_chooser_get_type ())
#define XFCE_IMAGE_FILE_CHOOSER(obj)	         (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_XFCE_IMAGE_FILE_CHOOSER, XfceImageFileChooser))
#define XFCE_IMAGE_FILE_CHOOSER_CLASS(klass)	 (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_XFCE_IMAGE_FILE_CHOOSER, XfceImageFileChooserClass))
#define IS_XFCE_IMAGE_FILE_CHOOSER(obj)	      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_XFCE_IMAGE_FILE_CHOOSER))
#define IS_XFCE_IMAGE_FILE_CHOOSER_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_XFCE_IMAGE_FILE_CHOOSER))
#define XFCE_IMAGE_FILE_CHOOSER_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_XFCE_IMAGE_FILE_CHOOSER, XfceImageFileChooserClass))

typedef struct _XfceImageFileChooser	    XfceImageFileChooser;
typedef struct _XfceImageFileChooserClass       XfceImageFileChooserClass;

typedef struct _XfceImageFileChooserPrivate     XfceImageFileChooserPrivate;

struct _XfceImageFileChooser
{
	XfceFileChooser parent;
	
	XfceImageFileChooserPrivate *priv;
};

struct _XfceImageFileChooserClass
{
	XfceFileChooserClass parent_class;
};


GType xfce_image_file_chooser_get_type(void) G_GNUC_CONST;

XfceImageFileChooser* xfce_image_file_chooser_new(void);
G_END_DECLS

#endif /* __XFCE_IMAGE_FILE_CHOOSER_H__ */
