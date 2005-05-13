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
#include <gtk/gtk.h>

#include "xfce-cell-renderer-pixbuf-on-demand.h"
#include "aspect-scale.h"

enum {
	LAST_SIGNAL
};

enum {
	PATH_PROP = 1,
	LOADED_PROP = 2,
	ITER_PROP = 3,
	PIXBUF_PROP = 4,
	FALLBACK_PIXBUF_PROP = 5,
	LAST_PROP = 5
};

struct _XfceCellRendererPixbufOnDemandPrivate
{
	gchar *path;
	gboolean loaded;
	GtkTreeIter *iter;
	GtkListStore *model;
	/*GdkPixbuf *dummypix;*/
	GdkPixbuf *pixbuf;
	GdkPixbuf *fallback_pixbuf;
	gint thumb_colid;
	gint loaded_colid;
	gint width;
	gint height;
};

static GtkCellRendererClass *parent_class = NULL;

/* static guint XfceCellRendererPixbufOnDemand_signals[LAST_SIGNAL] = { 0 }; */

static void xfce_cell_renderer_pixbuf_on_demand_class_init(XfceCellRendererPixbufOnDemandClass *klass);
static void xfce_cell_renderer_pixbuf_on_demand_init(XfceCellRendererPixbufOnDemand *aXfceCellRendererPixbufOnDemand);
static void xfce_cell_renderer_pixbuf_on_demand_finalize(GObject *object);
static void xfce_cell_renderer_pixbuf_on_demand_dispose(GObject *object);

static void xfce_cell_renderer_pixbuf_on_demand_get_property(GObject *object, guint param_id, GValue *value, GParamSpec *pspec);
static void xfce_cell_renderer_pixbuf_on_demand_set_property(GObject *object, guint param_id, const GValue *value, GParamSpec *pspec);

GType
xfce_cell_renderer_pixbuf_on_demand_get_type(void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo our_info =
			{
				sizeof(XfceCellRendererPixbufOnDemandClass),
				NULL,	   /* base_init */
				NULL,	   /* base_finalize */
				(GClassInitFunc) xfce_cell_renderer_pixbuf_on_demand_class_init,
				NULL,	   /* class_finalize */
				NULL,	   /* class_data */
				sizeof(XfceCellRendererPixbufOnDemand),
				0,	      /* n_preallocs */
				(GInstanceInitFunc) xfce_cell_renderer_pixbuf_on_demand_init
			};
		
		type = g_type_register_static(GTK_TYPE_CELL_RENDERER,
					      "XfceCellRendererPixbufOnDemand",
					      &our_info,
					      0);
	}
	
	return type;
}
static void 
xfce_cell_renderer_pixbuf_on_demand_finalize(GObject *object)
{
	XfceCellRendererPixbufOnDemand *aXfceCellRendererPixbufOnDemand;
	XfceCellRendererPixbufOnDemandPrivate *priv;

	g_return_if_fail(object != NULL);
	g_return_if_fail(IS_XFCE_CELL_RENDERER_PIXBUF_ON_DEMAND(object));

	aXfceCellRendererPixbufOnDemand = XFCE_CELL_RENDERER_PIXBUF_ON_DEMAND(object);
	priv = aXfceCellRendererPixbufOnDemand->priv;
	
	if (priv->iter) {
		gtk_tree_iter_free (priv->iter);
		priv->iter = NULL;
	}

	if (priv->pixbuf) {
		g_object_unref (G_OBJECT (priv->pixbuf));
		priv->pixbuf = NULL;
	}
	
	if (priv->fallback_pixbuf) {
		g_object_unref (G_OBJECT (priv->fallback_pixbuf));
		priv->fallback_pixbuf = NULL;
	}

	if (priv->path) {
		g_free (priv->path);
		priv->path = NULL;
	}

#if 0
	if (priv->dummypix) {
		g_object_unref (G_OBJECT (priv->dummypix));
		priv->dummypix = NULL;
	}
#endif
	
	if (G_OBJECT_CLASS(parent_class)->finalize)
	        (* G_OBJECT_CLASS(parent_class)->finalize)(object);

	g_free(priv);
}
static void 
xfce_cell_renderer_pixbuf_on_demand_dispose(GObject *object)
{
	XfceCellRendererPixbufOnDemand *aXfceCellRendererPixbufOnDemand;

	g_return_if_fail(object != NULL);
	g_return_if_fail(IS_XFCE_CELL_RENDERER_PIXBUF_ON_DEMAND(object));

	aXfceCellRendererPixbufOnDemand = XFCE_CELL_RENDERER_PIXBUF_ON_DEMAND(object);

	if (G_OBJECT_CLASS(parent_class)->dispose)
	        (* G_OBJECT_CLASS(parent_class)->dispose)(object);
}
static void
xfce_cell_renderer_pixbuf_on_demand_get_property(GObject *object, guint param_id, GValue *value, GParamSpec *pspec)
{
	XfceCellRendererPixbufOnDemand *aXfceCellRendererPixbufOnDemand;        
	XfceCellRendererPixbufOnDemandPrivate *priv;

	aXfceCellRendererPixbufOnDemand = XFCE_CELL_RENDERER_PIXBUF_ON_DEMAND(object);
	priv = aXfceCellRendererPixbufOnDemand->priv;

	switch (param_id) {
	case PATH_PROP:
		g_value_set_string (value, priv->path);
		break;
	case LOADED_PROP:
		g_value_set_boolean (value, priv->loaded);
		break;
	case ITER_PROP:
		g_value_set_boxed (value, priv->iter);
		break;
	case PIXBUF_PROP:
		g_value_set_object (value, G_OBJECT (priv->pixbuf));
		break;
	case FALLBACK_PIXBUF_PROP:
		g_value_set_object (value, G_OBJECT (priv->fallback_pixbuf));
		break;
	default:
	        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
	        break;
	}
}
static void
xfce_cell_renderer_pixbuf_on_demand_set_property(GObject *object, guint param_id, const GValue *value, GParamSpec *pspec)
{
	XfceCellRendererPixbufOnDemand *aXfceCellRendererPixbufOnDemand;        
	XfceCellRendererPixbufOnDemandPrivate *priv;
	gchar const *s;
	GdkPixbuf *pix;

	aXfceCellRendererPixbufOnDemand = XFCE_CELL_RENDERER_PIXBUF_ON_DEMAND(object);
	priv = aXfceCellRendererPixbufOnDemand->priv;

	switch (param_id) {
	case PATH_PROP:
		if (priv->path) {
			g_free (priv->path);
			priv->path = NULL;
		}
	
		/*g_value_set_string (value, priv->);	*/
		s = g_value_get_string (value);
		if (s) 
			priv->path = g_strdup (s);
		break;

	case LOADED_PROP:
		priv->loaded = g_value_get_boolean (value);
		break;

	case ITER_PROP:
		if (priv->iter) {
			gtk_tree_iter_free (priv->iter);
			priv->iter = NULL;
		}
		
		priv->iter = g_memdup (g_value_get_boxed (value), sizeof(GtkTreeIter));
		break;

	case FALLBACK_PIXBUF_PROP:
		if (priv->fallback_pixbuf) {
			g_object_unref (G_OBJECT (priv->fallback_pixbuf));
			priv->fallback_pixbuf = NULL;
		}
		
		priv->fallback_pixbuf = GDK_PIXBUF (g_value_dup_object (value));
		break;
		
	case PIXBUF_PROP:
		if (priv->pixbuf) {
			g_object_unref (G_OBJECT (priv->pixbuf));
			priv->pixbuf = NULL;
		}
		
		/*pix = g_value_get_object (value);*/
		/* FIXME use dup_object instead of _copy? */
		/*if (pix)
			priv->pixbuf = gdk_pixbuf_copy (pix);
		*/
		priv->pixbuf = GDK_PIXBUF (g_value_dup_object (value));
		break;
		
	default:
	        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
	        break;
	}
}

static void
xfce_cell_renderer_pixbuf_on_demand_get_size(GtkCellRenderer *cr, 
	GtkWidget *widget,
	GdkRectangle *cell_area,
	gint *x_offset,
	gint *y_offset,
	gint *width,
	gint *height)
{
	g_return_if_fail (GTK_IS_CELL_RENDERER (cr));

	XfceCellRendererPixbufOnDemandPrivate *priv;
	priv = XFCE_CELL_RENDERER_PIXBUF_ON_DEMAND (cr)->priv;
	
	if (x_offset)
		*x_offset = 0;
	
	if (y_offset)
		*y_offset = 0;
		
	*width = priv->width;
	*height = priv->height;
}

static void
xfce_cell_renderer_pixbuf_on_demand_render(GtkCellRenderer *cr,
	GdkWindow *window,
	GtkWidget *widget,
	GdkRectangle *background_area,
	GdkRectangle *cell_area,
	GdkRectangle *expose_area,
	GtkCellRendererState flags)
{
	XfceCellRendererPixbufOnDemandPrivate *priv;
	GdkPixbuf *pix;
	GdkRectangle r;
	GdkRectangle draw_rect;
	GdkGC *gc;
	
	priv = XFCE_CELL_RENDERER_PIXBUF_ON_DEMAND (cr)->priv;
	if (!priv->loaded)
		xfce_cell_renderer_pixbuf_on_demand_load (XFCE_CELL_RENDERER_PIXBUF_ON_DEMAND (cr), GDK_INTERP_NEAREST);
		
	if (priv->pixbuf)
		pix = priv->pixbuf;
	else if (priv->fallback_pixbuf)
		pix = priv->fallback_pixbuf;
	else
		pix = NULL; /*priv->dummypix;*/

	r.x = 0;
	r.y = 0;
	r.width = priv->width;
	r.height = priv->height;
	
	gc = gdk_gc_new (window);
	r.x = r.x + cell_area->x;
	r.y = r.y + cell_area->y;
	r.width = r.width - 0; /* # xpad * 2; */
	r.height = r.height - 0; /* # ypad * 2; */

	if (pix && gdk_rectangle_intersect(cell_area, &r, &draw_rect) &&
	gdk_rectangle_intersect(expose_area, &draw_rect, &draw_rect))
		gdk_draw_pixbuf (window, gc, pix,
			draw_rect.x - r.x,
			draw_rect.y - r.y,
			draw_rect.x, draw_rect.y,
			draw_rect.width,
			draw_rect.height,
			GDK_RGB_DITHER_NONE, 0, 0
		);
	

	g_object_unref (G_OBJECT (gc));		
/*	if (GTK_CELL_RENDERER (pb)->render) 
		GTK_CELL_RENDERER (pb)->render (pb, window, widget, background_area, cell_area, expose_area, flags);
*/
}

static gpointer
pboditer_copy_cb(gpointer boxed)
{
	return gtk_tree_iter_copy ((GtkTreeIter*)boxed);
}

static void
pboditer_free_cb(gpointer boxed)
{
	gtk_tree_iter_free ((GtkTreeIter*)boxed);
}


static void
xfce_cell_renderer_pixbuf_on_demand_class_init(XfceCellRendererPixbufOnDemandClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	GtkCellRendererClass *cr_class;
	static GType iter_boxed = 0;

	parent_class = g_type_class_peek_parent(klass);
	
	object_class->finalize = xfce_cell_renderer_pixbuf_on_demand_finalize;
	object_class->dispose = xfce_cell_renderer_pixbuf_on_demand_dispose;
	object_class->get_property = xfce_cell_renderer_pixbuf_on_demand_get_property;
	object_class->set_property = xfce_cell_renderer_pixbuf_on_demand_set_property;
	
	cr_class = GTK_CELL_RENDERER_CLASS(klass);
	
	cr_class->get_size = xfce_cell_renderer_pixbuf_on_demand_get_size;
	cr_class->render = xfce_cell_renderer_pixbuf_on_demand_render;

	g_object_class_install_property (object_class, PATH_PROP, 
		g_param_spec_string ("path", "Filesystem path to image", 
			"The path to the image to be loaded on demand",
			NULL,
			G_PARAM_READABLE|G_PARAM_WRITABLE
		)	
	);

	g_object_class_install_property (object_class, LOADED_PROP, 
		g_param_spec_boolean ("loaded", "Flag if loaded",
			"Flag if the pixbuf has been loaded or not",
			FALSE,
			G_PARAM_READABLE|G_PARAM_WRITABLE
		)	
	);

	
	if (!iter_boxed) {
		iter_boxed = g_boxed_type_register_static ("pboditer", pboditer_copy_cb, pboditer_free_cb);
	}

	g_object_class_install_property (object_class, ITER_PROP, 
		g_param_spec_boxed ("iter", "row iter",
			"The iter of the row to fill loaded and pixbuf in", 
			iter_boxed,
			G_PARAM_READABLE|G_PARAM_WRITABLE
		)	
	);

	g_object_class_install_property (object_class, PIXBUF_PROP, 
		g_param_spec_object ("pixbuf", "pixbuf to show",
			"The loaded pixbuf, if any",
			GDK_TYPE_PIXBUF,
			G_PARAM_READABLE|G_PARAM_WRITABLE
		)	
	);
}
static void 
xfce_cell_renderer_pixbuf_on_demand_init(XfceCellRendererPixbufOnDemand *aXfceCellRendererPixbufOnDemand)
{
	XfceCellRendererPixbufOnDemandPrivate *priv;

	priv = g_new0(XfceCellRendererPixbufOnDemandPrivate, 1);

	priv->thumb_colid = -1;
	priv->loaded_colid = -1;

	aXfceCellRendererPixbufOnDemand->priv = priv;
}
XfceCellRendererPixbufOnDemand*
xfce_cell_renderer_pixbuf_on_demand_new(void)
{
	XfceCellRendererPixbufOnDemand *aXfceCellRendererPixbufOnDemand;

	aXfceCellRendererPixbufOnDemand = g_object_new(xfce_cell_renderer_pixbuf_on_demand_get_type(), NULL);
	

	return aXfceCellRendererPixbufOnDemand;
}

void
xfce_cell_renderer_pixbuf_on_demand_set_model (XfceCellRendererPixbufOnDemand *r, GtkListStore *treestore)
{
	XfceCellRendererPixbufOnDemandPrivate *priv;
	priv = r->priv;
	priv->model = treestore;
}

void
xfce_cell_renderer_pixbuf_on_demand_set_size (XfceCellRendererPixbufOnDemand *r, gint width, gint height)
{
	XfceCellRendererPixbufOnDemandPrivate *priv;
	priv = r->priv;
	priv->width = width;
	priv->height = height;
	
#if 0
	if (priv->dummypix) {
		g_object_unref (G_OBJECT (priv->dummypix));
		priv->dummypix = NULL;
	}
	
	priv->dummypix = create_empty_pixbuf (priv->width, priv->height);
#endif
}

void
xfce_cell_renderer_pixbuf_on_demand_load(XfceCellRendererPixbufOnDemand *r, GdkInterpType p)
{
	XfceCellRendererPixbufOnDemandPrivate *priv;
	GdkPixbuf *pix;
	GdkPixbuf *npix;
	
	priv = r->priv;
	
	if (!priv->path)
		return;

	g_object_set (G_OBJECT (r), "loaded", TRUE, NULL);

	pix = gdk_pixbuf_new_from_file (priv->path, NULL);
	npix = aspect_scale_simple (pix, priv->width, priv->height, p);
	if (pix)
		g_object_unref (G_OBJECT (pix));
		
	pix = npix;
	npix = NULL;
	
	g_object_set (G_OBJECT (r), "pixbuf", pix, NULL);
	
	if (priv->model) {
		if (priv->loaded_colid > -1)
			gtk_list_store_set (priv->model, priv->iter, priv->loaded_colid, TRUE, -1);

		if (priv->thumb_colid > -1) {
			gtk_list_store_set (priv->model, priv->iter, priv->thumb_colid, pix, -1);
		}
	}

	g_object_unref (pix);
}

void xfce_cell_renderer_pixbuf_on_demand_set_loaded_column (XfceCellRendererPixbufOnDemand *r, gint colid)
{
	XfceCellRendererPixbufOnDemandPrivate *priv;
	priv = r->priv;
	priv->loaded_colid = colid;
}

void xfce_cell_renderer_pixbuf_on_demand_set_thumb_column (XfceCellRendererPixbufOnDemand *r, gint colid)
{
	XfceCellRendererPixbufOnDemandPrivate *priv;
	priv = r->priv;
	priv->thumb_colid = colid;
}

