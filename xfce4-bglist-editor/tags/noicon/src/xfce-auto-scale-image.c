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
#include <gdk/gdkpixbuf.h>
#include <gtk/gtk.h>
#include "xfce-auto-scale-image.h"
#include "aspect-scale.h"

enum {
	LAST_SIGNAL
};

enum {
	ORIGPIXBUF_PROP = 1,
	LAST_PROP
};

struct _XfceAutoScaleImagePrivate
{
	GdkPixbuf *origpixbuf;
	int currentsize[2];
};

static GtkImageClass *parent_class = NULL;

/* static guint xfce_auto_scale_image_signals[LAST_SIGNAL] = { 0 }; */

static void xfce_auto_scale_image_class_init(XfceAutoScaleImageClass *klass);
static void xfce_auto_scale_image_init(XfceAutoScaleImage *aXfceAutoScaleImage);
static void xfce_auto_scale_image_finalize(GObject *object);
static void xfce_auto_scale_image_dispose(GObject *object);

static void xfce_auto_scale_image_get_property(GObject *object, guint param_id, GValue *value, GParamSpec *pspec);
static void xfce_auto_scale_image_set_property(GObject *object, guint param_id, const GValue *value, GParamSpec *pspec);

GType
xfce_auto_scale_image_get_type(void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo our_info =
			{
				sizeof(XfceAutoScaleImageClass),
				NULL,	   /* base_init */
				NULL,	   /* base_finalize */
				(GClassInitFunc) xfce_auto_scale_image_class_init,
				NULL,	   /* class_finalize */
				NULL,	   /* class_data */
				sizeof(XfceAutoScaleImage),
				0,	      /* n_preallocs */
				(GInstanceInitFunc) xfce_auto_scale_image_init
			};
		
		type = g_type_register_static(GTK_TYPE_IMAGE,
					      "XfceAutoScaleImage",
					      &our_info,
					      0);
	}
	
	return type;
}
static void 
xfce_auto_scale_image_finalize(GObject *object)
{
	XfceAutoScaleImage *aXfceAutoScaleImage;
	XfceAutoScaleImagePrivate *priv;


	g_return_if_fail(object != NULL);
	g_return_if_fail(IS_XFCE_AUTO_SCALE_IMAGE(object));

	aXfceAutoScaleImage = XFCE_AUTO_SCALE_IMAGE(object);
	priv  =aXfceAutoScaleImage->priv;
	if (priv && priv->origpixbuf) {
		g_object_unref (G_OBJECT (priv->origpixbuf));
		priv->origpixbuf = NULL;
	}

	if (G_OBJECT_CLASS(parent_class)->finalize)
	        (* G_OBJECT_CLASS(parent_class)->finalize)(object);

	g_free(aXfceAutoScaleImage->priv);
}
static void 
xfce_auto_scale_image_dispose(GObject *object)
{
	XfceAutoScaleImage *aXfceAutoScaleImage;

	g_return_if_fail(object != NULL);
	g_return_if_fail(IS_XFCE_AUTO_SCALE_IMAGE(object));

	aXfceAutoScaleImage = XFCE_AUTO_SCALE_IMAGE(object);

	if (G_OBJECT_CLASS(parent_class)->dispose)
	        (* G_OBJECT_CLASS(parent_class)->dispose)(object);
}
static void
xfce_auto_scale_image_get_property(GObject *object, guint param_id, GValue *value, GParamSpec *pspec)
{
	XfceAutoScaleImage *aXfceAutoScaleImage;        
	XfceAutoScaleImagePrivate *priv;

	aXfceAutoScaleImage = XFCE_AUTO_SCALE_IMAGE(object);
	priv = aXfceAutoScaleImage->priv;

	switch (param_id) {
	case ORIGPIXBUF_PROP:
		g_value_set_object (value, priv->origpixbuf);
		break;
	default:
	        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
	        break;
	}
}
static void
xfce_auto_scale_image_set_property(GObject *object, guint param_id, const GValue *value, GParamSpec *pspec)
{
	XfceAutoScaleImage *aXfceAutoScaleImage;        
	XfceAutoScaleImagePrivate *priv;
	gpointer p;

	aXfceAutoScaleImage = XFCE_AUTO_SCALE_IMAGE(object);
	priv = aXfceAutoScaleImage->priv;

	switch (param_id) {
	case ORIGPIXBUF_PROP:
		if (priv->origpixbuf) {
			g_object_unref (G_OBJECT (priv->origpixbuf));
			priv->origpixbuf = NULL;
		}
		p = g_value_get_object (value);
		                    
		if (p) {
			g_return_if_fail (GDK_IS_PIXBUF (p));
			g_object_ref (G_OBJECT (p));
			priv->origpixbuf = GDK_PIXBUF (p);
		}
		break;
	default:
	        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
	        break;
	}
}

static void
xfce_auto_scale_image_size_allocate(GtkWidget *widget, GtkAllocation *allocation) /*, gpointer user_data)*/
{
	GdkPixbuf *pix;
	GdkPixbuf *origpixbuf;
	XfceAutoScaleImage *aXfceAutoScaleImage;
	XfceAutoScaleImagePrivate *priv;
	aXfceAutoScaleImage = XFCE_AUTO_SCALE_IMAGE (widget);
	
	priv = aXfceAutoScaleImage->priv;
	if (allocation->width != priv->currentsize[0] || allocation->height != priv->currentsize[1]) {
		pix = aspect_scale_simple(priv->origpixbuf, allocation->width, allocation->height, GDK_INTERP_BILINEAR);
		priv->currentsize[0] = allocation->width;
		priv->currentsize[1] = allocation->height;

		gtk_image_set_from_pixbuf (GTK_IMAGE (aXfceAutoScaleImage), pix);
		g_object_unref (G_OBJECT (pix));
	}

	if (GTK_WIDGET_CLASS(parent_class)->size_allocate)
	        (* GTK_WIDGET_CLASS(parent_class)->size_allocate)(GTK_WIDGET(aXfceAutoScaleImage), allocation);
}


static void
xfce_auto_scale_image_class_init(XfceAutoScaleImageClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

	parent_class = g_type_class_peek_parent(klass);
	
	object_class->finalize = xfce_auto_scale_image_finalize;
	object_class->dispose = xfce_auto_scale_image_dispose;
	object_class->get_property = xfce_auto_scale_image_get_property;
	object_class->set_property = xfce_auto_scale_image_set_property;

	widget_class->size_allocate = xfce_auto_scale_image_size_allocate;
	
	g_object_class_install_property (object_class, ORIGPIXBUF_PROP, 
		g_param_spec_object ("origpixbuf", "Unscaled Pixbuf", 
			"The original, unscaled pixbuf", 
			gdk_pixbuf_get_type (),
			G_PARAM_READABLE|G_PARAM_WRITABLE
		)	
	);
}
static void 
xfce_auto_scale_image_init(XfceAutoScaleImage *aXfceAutoScaleImage)
{
	XfceAutoScaleImagePrivate *priv;

	priv = g_new0(XfceAutoScaleImagePrivate, 1);

	aXfceAutoScaleImage->priv = priv;
	priv->origpixbuf = NULL;
	priv->currentsize[0] = 0;
	priv->currentsize[1] = 0;
	
	/*g_object_connect (G_OBJECT (aXfceAutoScaleImage), "notify::pixbuf", 
		G_CALLBACK(xfce_auto_scale_image_pixbuf_changed_cb)
	);*/
}

/*
static void
xfce_auto_scale_image_pixbuf_changed_cb(
	XfceAutoScaleImage *image,
	GParamSpec *spec
)
{
	GdkPixbuf *pix;
	g_object_get (G_OBJECT (image), pspec->name, &pix, NULL);
	g_object_set (G_OBJECT (image), "origpixbuf", pix, NULL);
	g_object_unref (G_OBJECT (pix));
}
*/

XfceAutoScaleImage*
xfce_auto_scale_image_new(void)
{
	XfceAutoScaleImage *aXfceAutoScaleImage;
	aXfceAutoScaleImage = g_object_new(xfce_auto_scale_image_get_type(), NULL);
	return aXfceAutoScaleImage;
}


void 
xfce_auto_scale_image_set_from_pixbuf(XfceAutoScaleImage *image, GdkPixbuf *pixbuf)
{
	gtk_image_set_from_pixbuf (GTK_IMAGE (image), pixbuf);
	g_object_set (G_OBJECT (image), "origpixbuf", pixbuf, NULL);
}

void xfce_auto_scale_image_set_from_file(XfceAutoScaleImage *image, const gchar *filename)
{
	GdkPixbuf *pix;
	pix = gdk_pixbuf_new_from_file (filename, NULL);
	xfce_auto_scale_image_set_from_pixbuf (image, pix);
	g_object_unref (G_OBJECT (pix));
}
