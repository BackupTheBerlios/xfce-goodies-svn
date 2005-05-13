#ifndef __ASPECTSCALE_H
#define __ASPECTSCALE_H
#include <gdk/gdk.h>

GdkPixbuf *create_empty_pixbuf(gint dest_width, gint dest_height);
GdkPixbuf *aspect_scale_simple(GdkPixbuf *src, gint dest_width, gint dest_height, GdkInterpType interp_type);

#endif

