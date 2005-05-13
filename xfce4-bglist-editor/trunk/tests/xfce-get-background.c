#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

/* TODO:

The XGetWindowProperty() function that gdk_property_get() uses has a 
very confusing and complicated set of semantics. Unfortunately, 
gdk_property_get() makes the situation worse instead of better 
(the semantics should be considered undefined), and also prints 
warnings to stderr in cases where it should return a useful error 
to the program. You are advised to use XGetWindowProperty() directly 
until a replacement function for gdk_property_get() is provided.

*/

gchar *xfce_get_current_background_image()
{
	GdkAtom atom;
	gint fmt;
	gint len;
	/*guchar buf[2049];*/
	guchar *buf;
	buf = NULL;
	if (gdk_property_get(
		gdk_screen_get_root_window(
gdk_display_get_default_screen(gdk_display_get_default())		
		),
		gdk_atom_intern("XFDESKTOP_IMAGE_FILE_0", FALSE),
		gdk_x11_xatom_to_atom(XA_STRING), 
		0, 2048, 0, 
		&atom, &fmt, &len, 
		&buf) && fmt == 8) 
	{
		buf[len] = 0;
		return buf;
	}
	return NULL;
}
