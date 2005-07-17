/*
 * Copyright (c) 2005 Daniel Bobadilla Leal <dbobadil@dcc.uchile.cl>
 * Copyright (c) 2003 Benedikt Meurer <benedikt.meurer@unix-ag.uni-siegen.de>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <libxfce4util/i18n.h>
#include <libxfcegui4/dialogs.h>
#include <panel/plugins.h>
#include <panel/xfce.h>

#include "shooter_icon.h"

typedef struct
{
  GtkWidget *ebox;
  GtkWidget *button;
} t_screenshooter;

t_screenshooter *screenshooter;

static void
shootthemup (GtkWidget * parent, gpointer data)
{
  gchar *filename;
  GtkFileChooser *chooser;
  GdkPixbuf *screen; 

  filename = NULL;
  /*For Now without thumbnails these could be read from config */
  screen = take_screenshot (0, 0);

  chooser = xfce_file_chooser_dialog_new ("Save Screenshot",
					  NULL,
					  XFCE_FILE_CHOOSER_ACTION_SAVE,
					  GTK_STOCK_CANCEL,
					  GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE,
					  GTK_RESPONSE_ACCEPT, NULL);
  filename = generate_filename_for_uri (xfce_file_chooser_get_current_folder(XFCE_FILE_CHOOSER (chooser))) ;
  if(filename == NULL) 
	  filename = g_strdup_printf (_("Screenshot.png"));
  xfce_file_chooser_set_current_name (XFCE_FILE_CHOOSER (chooser),
				      filename);


  if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
    {
      filename = xfce_file_chooser_get_filename (XFCE_FILE_CHOOSER (chooser));

  		if (filename != NULL && strlen (filename) > 0)
    		{
					save_screenshot(screen,filename);
      				g_free (filename);
    		}
    }

  gtk_widget_destroy ((GtkWidget *) chooser);


return ;

}

static t_screenshooter *
screenshooter_new (void)
{

  GtkWidget *image;
  GdkPixbuf *pixbuf;
  GtkTooltips *tooltips;
  GtkWidget *box;
  tooltips = gtk_tooltips_new ();
  screenshooter = g_new (t_screenshooter, 1);
  box = gtk_hbox_new (TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (box), 0);

  screenshooter->ebox = gtk_event_box_new ();
  gtk_widget_show (screenshooter->ebox);
  screenshooter->button = gtk_button_new ();
  gtk_button_set_relief(GTK_BUTTON(screenshooter->button),GTK_RELIEF_NONE);

  pixbuf = gdk_pixbuf_new_from_xpm_data (CAMERA_ICON);
  /*Fun tooltip :) */

  gtk_tooltips_set_tip (tooltips, screenshooter->ebox,
			_("Take an Screenshot"), NULL);
  /*Set the icon */

  if (pixbuf != NULL)
    {
      image = gtk_image_new_from_pixbuf (pixbuf);
      gtk_box_pack_start (GTK_BOX (box), image, FALSE, FALSE, 1);
      gtk_widget_show (image);
      gtk_widget_show (box);
      gtk_container_add (GTK_CONTAINER (screenshooter->button), box);
    }

  gtk_widget_show (screenshooter->button);
  gtk_container_add (GTK_CONTAINER (screenshooter->ebox),
		     screenshooter->button);

  g_signal_connect (screenshooter->button, "clicked",
		    G_CALLBACK (shootthemup), screenshooter);

  return (screenshooter);
}

static gboolean
screenshooter_control_new (Control * ctrl)
{
  t_screenshooter *screenshooter;

  screenshooter = screenshooter_new ();

  gtk_container_add (GTK_CONTAINER (ctrl->base), screenshooter->ebox);

  ctrl->data = (gpointer) screenshooter;
  ctrl->with_popup = FALSE;

  gtk_widget_set_size_request (ctrl->base, -1, -1);

  return (TRUE);
}

static void
screenshooter_free (Control * ctrl)
{
  t_screenshooter *screenshooter;

  g_return_if_fail (ctrl != NULL);
  g_return_if_fail (ctrl->data != NULL);

  screenshooter = (t_screenshooter *) ctrl->data;

  g_free (screenshooter);
}

static void
screenshooter_read_config (Control * ctrl, xmlNodePtr parent)
{
  /* do something useful here */
}

static void
screenshooter_write_config (Control * ctrl, xmlNodePtr parent)
{
  /* do something useful here */
}

static void
screenshooter_attach_callback (Control * ctrl, const gchar * signal,
			       GCallback cb, gpointer data)
{
  t_screenshooter *screenshooter1;

  screenshooter1 = (t_screenshooter *) ctrl->data;
  g_signal_connect (screenshooter1->ebox, signal, cb, data);
  g_signal_connect (screenshooter1->button, signal, cb, data);
}

static void
screenshooter_set_size (Control * ctrl, int size)
{

  gtk_widget_set_size_request (screenshooter->ebox, 46 , 32);

  return;


}

/* options dialog */
static void
create_options (Control * ctrl, GtkContainer * con, GtkWidget * done)
{
}

/* initialization */
G_MODULE_EXPORT void
xfce_control_class_init (ControlClass * cc)
{
  /* these are required */
  cc->name = "screenshooter";
  cc->caption = _("Screenshooter");

  cc->create_control = (CreateControlFunc) screenshooter_control_new;

  cc->free = screenshooter_free;
  cc->attach_callback = screenshooter_attach_callback;

  /* options; don't define if you don't have any ;) */
  /*      cc->read_config         = screenshooter_read_config;
     cc->write_config     = screenshooter_write_config;
     cc->create_options           = screenshooter_create_options; */

  /* Don't use this function at all if you want xfce to
   * do the sizing.
   * Just define the set_size function to NULL, or rather, don't 
   * set it to something else.
   */
  cc->set_size = screenshooter_set_size;

  /* unused in the screenshooter:
   * ->set_orientation
   * ->set_theme
   */

}

/* required! defined in panel/plugins.h */
XFCE_PLUGIN_CHECK_INIT
