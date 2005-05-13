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
#ifndef __XFCE_CELL_RENDERER_PIXBUF_ON_DEMAND_H__
#define __XFCE_CELL_RENDERER_PIXBUF_ON_DEMAND_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

/* properties:
	path
	loaded
	iter
	pixbuf
*/


#define TYPE_XFCE_CELL_RENDERER_PIXBUF_ON_DEMAND	         (xfce_cell_renderer_pixbuf_on_demand_get_type ())
#define XFCE_CELL_RENDERER_PIXBUF_ON_DEMAND(obj)	         (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_XFCE_CELL_RENDERER_PIXBUF_ON_DEMAND, XfceCellRendererPixbufOnDemand))
#define XFCE_CELL_RENDERER_PIXBUF_ON_DEMAND_CLASS(klass)	 (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_XFCE_CELL_RENDERER_PIXBUF_ON_DEMAND, XfceCellRendererPixbufOnDemandClass))
#define IS_XFCE_CELL_RENDERER_PIXBUF_ON_DEMAND(obj)	      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_XFCE_CELL_RENDERER_PIXBUF_ON_DEMAND))
#define IS_XFCE_CELL_RENDERER_PIXBUF_ON_DEMAND_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_XFCE_CELL_RENDERER_PIXBUF_ON_DEMAND))
#define XFCE_CELL_RENDERER_PIXBUF_ON_DEMAND_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_XFCE_CELL_RENDERER_PIXBUF_ON_DEMAND, XfceCellRendererPixbufOnDemandClass))

typedef struct _XfceCellRendererPixbufOnDemand	    XfceCellRendererPixbufOnDemand;
typedef struct _XfceCellRendererPixbufOnDemandClass       XfceCellRendererPixbufOnDemandClass;

typedef struct _XfceCellRendererPixbufOnDemandPrivate     XfceCellRendererPixbufOnDemandPrivate;

struct _XfceCellRendererPixbufOnDemand
{
	GtkCellRenderer parent;
	
	XfceCellRendererPixbufOnDemandPrivate *priv;
};

struct _XfceCellRendererPixbufOnDemandClass
{
	GtkCellRendererClass parent_class;
};


GType xfce_cell_renderer_pixbuf_on_demand_get_type(void) G_GNUC_CONST;

XfceCellRendererPixbufOnDemand* xfce_cell_renderer_pixbuf_on_demand_new(void);

void xfce_cell_renderer_pixbuf_on_demand_set_model (XfceCellRendererPixbufOnDemand *r, GtkListStore *treestore);
void xfce_cell_renderer_pixbuf_on_demand_set_size (XfceCellRendererPixbufOnDemand *r, gint width, gint height);
void xfce_cell_renderer_pixbuf_on_demand_set_loaded_column (XfceCellRendererPixbufOnDemand *r, gint colid);
void xfce_cell_renderer_pixbuf_on_demand_set_thumb_column (XfceCellRendererPixbufOnDemand *r, gint colid);
void xfce_cell_renderer_pixbuf_on_demand_load(XfceCellRendererPixbufOnDemand *r, GdkInterpType p);

G_END_DECLS

#endif /* __XFCE_CELL_RENDERER_PIXBUF_ON_DEMAND_H__ */
