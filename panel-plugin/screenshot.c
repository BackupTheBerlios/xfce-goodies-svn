/* Copyright (C) 2005 Daniel Bobadilla Leal <dbobadil@dcc.uchile.cl>
 * Copyright (C) 2004 German Poo-Caaman~o <gpoo@ubiobio.cl>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include <gdk/gdk.h>
#include <stdlib.h>
#include <libxfce4util/i18n.h>
#include <libxfcegui4/dialogs.h>
#include <panel/plugins.h>
#include <panel/xfce.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MODE 0644


GdkPixbuf * take_screenshot (gint new_width, gint new_height)
{
  gint width, height;
  GdkPixbuf *screenshot = NULL;
  GdkPixbuf *thumbnail = NULL;


  width = gdk_screen_width ();
  height = gdk_screen_height ();

  screenshot = gdk_pixbuf_get_from_drawable (NULL,
					     gdk_get_default_root_window
					     (), NULL, 0, 0, 0, 0,
					     width, height);

  if (new_width != 0 && new_height <= 0)
    {
      new_height = new_width * ((gfloat) height / (gfloat) width);
    }

  if (new_height != 0 && new_width <= 0)
    {
      new_width = new_height * ((gfloat) width / (gfloat) height);
    }

  if (new_height <= 0 && new_width <= 0)
    {
      new_height = height;
      new_width = width;
    }

  thumbnail = gdk_pixbuf_scale_simple (screenshot,
				       new_width,
				       new_height, GDK_INTERP_BILINEAR);
  return thumbnail;
}

gchar *generate_filename_for_uri(char *uri){
		int test;
		gchar *file_name;
		unsigned int i = 0;
		if(uri == NULL)
			return NULL;
		file_name = g_strdup (_("Screenshot.png"));
   if((test=open(file_name,O_RDWR,MODE))==-1)
   {
	return file_name;
   }
   do{
	   i++;
	   g_free (file_name);
	   file_name = g_strdup_printf (_("Screenshot-%d.png"),i);
   }
   while((test=open(file_name,O_RDWR,MODE))!=-1);

	return file_name;


}

void save_screenshot(GdkPixbuf *screenshot,gchar *file){
  GError *error = NULL;
  g_assert (file != NULL);
  
  gdk_pixbuf_save (screenshot, file, "png", &error, NULL);
  if (error != NULL)
    {
      xfce_err (error->message);
      g_error_free (error);
    }
}
