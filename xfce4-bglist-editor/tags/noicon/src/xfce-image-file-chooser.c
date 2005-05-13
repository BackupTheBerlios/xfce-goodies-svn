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

#include "xfce-image-file-chooser.h"

enum {
	LAST_SIGNAL
};

enum {
	LAST_PROP
};

struct _XfceImageFileChooserPrivate
{
};

static XfceFileChooserClass *parent_class = NULL;

/* static guint XfceImageFileChooser_signals[LAST_SIGNAL] = { 0 }; */

static void xfce_image_file_chooser_class_init(XfceImageFileChooserClass *klass);
static void xfce_image_file_chooser_init(XfceImageFileChooser *aXfceImageFileChooser);
static void xfce_image_file_chooser_finalize(GObject *object);
static void xfce_image_file_chooser_dispose(GObject *object);

static void xfce_image_file_chooser_get_property(GObject *object, guint param_id, GValue *value, GParamSpec *pspec);
static void xfce_image_file_chooser_set_property(GObject *object, guint param_id, const GValue *value, GParamSpec *pspec);

GType
xfce_image_file_chooser_get_type(void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo our_info =
			{
				sizeof(XfceImageFileChooserClass),
				NULL,	   /* base_init */
				NULL,	   /* base_finalize */
				(GClassInitFunc) xfce_image_file_chooser_class_init,
				NULL,	   /* class_finalize */
				NULL,	   /* class_data */
				sizeof(XfceImageFileChooser),
				0,	      /* n_preallocs */
				(GInstanceInitFunc) xfce_image_file_chooser_init
			};
		
		type = g_type_register_static(XFCE_TYPE_FILE_CHOOSER,
					      "XfceImageFileChooser",
					      &our_info,
					      0);
	}
	
	return type;
}
static void 
xfce_image_file_chooser_finalize(GObject *object)
{
	XfceImageFileChooser *aXfceImageFileChooser;

	g_return_if_fail(object != NULL);
	g_return_if_fail(IS_XFCE_IMAGE_FILE_CHOOSER(object));

	aXfceImageFileChooser = XFCE_IMAGE_FILE_CHOOSER(object);

	if (G_OBJECT_CLASS(parent_class)->finalize)
	        (* G_OBJECT_CLASS(parent_class)->finalize)(object);

	g_free(aXfceImageFileChooser->priv);
}
static void 
xfce_image_file_chooser_dispose(GObject *object)
{
	XfceImageFileChooser *aXfceImageFileChooser;

	g_return_if_fail(object != NULL);
	g_return_if_fail(IS_XFCE_IMAGE_FILE_CHOOSER(object));

	aXfceImageFileChooser = XFCE_IMAGE_FILE_CHOOSER(object);

	if (G_OBJECT_CLASS(parent_class)->dispose)
	        (* G_OBJECT_CLASS(parent_class)->dispose)(object);
}
static void
xfce_image_file_chooser_get_property(GObject *object, guint param_id, GValue *value, GParamSpec *pspec)
{
	XfceImageFileChooser *aXfceImageFileChooser;        

	aXfceImageFileChooser = XFCE_IMAGE_FILE_CHOOSER(object);

	switch (param_id) {
	default:
	        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
	        break;
	}
}
static void
xfce_image_file_chooser_set_property(GObject *object, guint param_id, const GValue *value, GParamSpec *pspec)
{
	XfceImageFileChooser *aXfceImageFileChooser;        

	aXfceImageFileChooser = XFCE_IMAGE_FILE_CHOOSER(object);

	switch (param_id) {
	default:
	        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
	        break;
	}
}

static void
xfce_image_file_chooser_class_init(XfceImageFileChooserClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	parent_class = g_type_class_peek_parent(klass);
	
	object_class->finalize = xfce_image_file_chooser_finalize;
	object_class->dispose = xfce_image_file_chooser_dispose;
	object_class->get_property = xfce_image_file_chooser_get_property;
	object_class->set_property = xfce_image_file_chooser_set_property;

}
static void 
xfce_image_file_chooser_init(XfceImageFileChooser *aXfceImageFileChooser)
{
	XfceImageFileChooserPrivate *priv;

	priv = g_new0(XfceImageFileChooserPrivate, 1);

	aXfceImageFileChooser->priv = priv;
}
XfceImageFileChooser*
xfce_image_file_chooser_new(void)
{
	XfceImageFileChooser *aXfceImageFileChooser;
	XfceImageFileChooserPrivate *priv;

	aXfceImageFileChooser = g_object_new(xfce_image_file_chooser_get_type(), NULL);
	
	priv = aXfceImageFileChooser->priv;

	return aXfceImageFileChooser;
}
