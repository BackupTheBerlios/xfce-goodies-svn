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
#ifndef __XFCE_AUTO_SCALE_IMAGE_H__
#define __XFCE_AUTO_SCALE_IMAGE_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TYPE_XFCE_AUTO_SCALE_IMAGE                 (xfce_auto_scale_image_get_type ())
#define XFCE_AUTO_SCALE_IMAGE(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_XFCE_AUTO_SCALE_IMAGE, XfceAutoScaleImage))
#define XFCE_AUTO_SCALE_IMAGE_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_XFCE_AUTO_SCALE_IMAGE, XfceAutoScaleImageClass))
#define IS_XFCE_AUTO_SCALE_IMAGE(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_XFCE_AUTO_SCALE_IMAGE))
#define IS_XFCE_AUTO_SCALE_IMAGE_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_XFCE_AUTO_SCALE_IMAGE))
#define XFCE_AUTO_SCALE_IMAGE_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_XFCE_AUTO_SCALE_IMAGE, XfceAutoScaleImageClass))

typedef struct _XfceAutoScaleImage            XfceAutoScaleImage;
typedef struct _XfceAutoScaleImageClass       XfceAutoScaleImageClass;

typedef struct _XfceAutoScaleImagePrivate     XfceAutoScaleImagePrivate;

struct _XfceAutoScaleImage
{
        GtkImage parent;
        
        XfceAutoScaleImagePrivate *priv;
};

struct _XfceAutoScaleImageClass
{
        GtkImageClass parent_class;
};


GType xfce_auto_scale_image_get_type(void) G_GNUC_CONST;

XfceAutoScaleImage* xfce_auto_scale_image_new(void);

void xfce_auto_scale_image_set_from_pixbuf(XfceAutoScaleImage *image, GdkPixbuf *pixbuf);
void xfce_auto_scale_image_set_from_file(XfceAutoScaleImage *image, const gchar *filename);

G_END_DECLS

#endif /* __XFCE_AUTO_SCALE_IMAGE_H__ */
