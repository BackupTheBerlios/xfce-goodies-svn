/*
 * Copyright (c) 2003 Patrick van Staveren <pvanstav@cs.wmich.edu>
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

#include <libxfce4util/i18n.h>
#include <libxfcegui4/dialogs.h>
#include <panel/plugins.h>
#include <panel/xfce.h>

#include <xmmsctrl.h>

#define PREV (DATA_DIR "/xmms-plugin-prev.png")
#define PLAY (DATA_DIR "/xmms-plugin-play.png")
#define PAUS (DATA_DIR "/xmms-plugin-pause.png")
#define STOP (DATA_DIR "/xmms-plugin-stop.png")
#define NEXT (DATA_DIR "/xmms-plugin-next.png")

typedef struct{
  GtkWidget *prev, *play, *pause, *stop, *next;    
} items;

gint session = 0;

static void prev(GtkWidget *widget, gpointer data){
  xmms_remote_playlist_prev(session);
}

static void play(GtkWidget *widget, gpointer data){
  xmms_remote_play(session);
}

static void paus(GtkWidget *widget, gpointer data){
  xmms_remote_pause(session);
}

static void stop(GtkWidget *widget, gpointer data){
  xmms_remote_stop(session);
}

static void next(GtkWidget *widget, gpointer data){
  xmms_remote_playlist_next(session);
}

static void tooltip_set(GtkWidget *widget, gpointer data){
  /* ToolTips dont work just yet, so its all commented out...expect them to work in a later release!
  g_print("in tooltip set\n");
  GtkTooltips *cur;
  cur = (GtkTooltips *) data;
  g_print("cur set to data");
  gtk_tooltips_set_tip(cur, widget,
			     xmms_remote_get_playlist_title
			     (session, xmms_remote_get_playlist_pos(session)), NULL);
  g_print("tooltip now set\n");
  g_free(cur);
  */
}

/*
static void callback(GtkWidget *widget, gpointer data){
  switch((gchar) data){
  case 'z': xmms_remote_playlist_prev(session); break;
  case 'x': xmms_remote_play(session);          break;
  case 'c': xmms_remote_pause(session);         break;
  case 'v': xmms_remote_stop(session);          break;
  case 'b': xmms_remote_playlist_next(session); break;
  default:  g_print("ERROR: INVALID CALLBACK SIGNAL %c", (gchar) data);
  }
}
*/
static GtkWidget *new_button_with_img(gchar *filename){
  GtkWidget *button, *image, *eventbox;

  image = gtk_image_new_from_file(filename);
  gtk_widget_show(image);
  
  eventbox = gtk_event_box_new();

  gtk_container_add(GTK_CONTAINER(eventbox), image);
  /*  button = gtk_button_new();
  gtk_container_add(GTK_CONTAINER (button), image);

  gtk_button_set_relief(button, GTK_RELIEF_NONE);

  gtk_widget_set_size_request(button, 18, 18);  
  */
  /*gtk_container_border_width(GTK_CONTAINER(button), 0);*/
  
  return(eventbox);
}

static gboolean xmms_plugin_control_new(Control *ctrl){
  GtkWidget *button, *box;
  GtkTooltips *curtrack;
  items *itembox;

  gint doexpand;
  gint dofill;
  gint padding;

  doexpand = TRUE;
  dofill = TRUE;
  padding = 1;

  itembox = g_new(items, 1);

  box = gtk_hbox_new(FALSE, 0);
  curtrack = gtk_tooltips_new();
  
  button = new_button_with_img(PREV);
  gtk_widget_set_events(button, GDK_BUTTON_PRESS_MASK);
  g_signal_connect(G_OBJECT(button), "button_press_event", G_CALLBACK(prev), NULL);
  g_signal_connect(G_OBJECT(button), "enter-notify-event", G_CALLBACK(tooltip_set), (gpointer) curtrack);
  gtk_box_pack_start(GTK_BOX(box), button, doexpand, dofill, padding);
  gtk_widget_show(button);
  itembox->prev = button;
  gtk_tooltips_set_tip(curtrack, button,
		       xmms_remote_get_playlist_title
		       (session, xmms_remote_get_playlist_pos(session)), NULL);

  button = new_button_with_img(PLAY);
  gtk_widget_set_events(button, GDK_BUTTON_PRESS_MASK);
  g_signal_connect(G_OBJECT(button), "button_press_event", G_CALLBACK(play), NULL);
  gtk_box_pack_start(GTK_BOX(box), button, doexpand, dofill, padding);
  gtk_widget_show(button);
  itembox->play = button;
  gtk_tooltips_set_tip(curtrack, button,
		       xmms_remote_get_playlist_title
		       (session, xmms_remote_get_playlist_pos(session)), NULL);

  button = new_button_with_img(PAUS);
  gtk_widget_set_events(button, GDK_BUTTON_PRESS_MASK);
  g_signal_connect(G_OBJECT(button), "button_press_event", G_CALLBACK(paus), NULL);
  gtk_box_pack_start(GTK_BOX(box), button, doexpand, dofill, padding);
  gtk_widget_show(button);
  itembox->pause = button;
  gtk_tooltips_set_tip(curtrack, button,
		       xmms_remote_get_playlist_title
		       (session, xmms_remote_get_playlist_pos(session)), NULL);

  button = new_button_with_img(STOP);
  gtk_widget_set_events(button, GDK_BUTTON_PRESS_MASK);
  g_signal_connect(G_OBJECT(button), "button_press_event", G_CALLBACK(stop), NULL);
  gtk_box_pack_start(GTK_BOX(box), button, doexpand, dofill, padding);
  gtk_widget_show(button);
  itembox->stop = button;
  gtk_tooltips_set_tip(curtrack, button,
		       xmms_remote_get_playlist_title
		       (session, xmms_remote_get_playlist_pos(session)), NULL);

  button = new_button_with_img( NEXT );
  gtk_widget_set_events(button, GDK_BUTTON_PRESS_MASK);
  g_signal_connect(G_OBJECT(button), "button_press_event", G_CALLBACK(next), NULL);
  gtk_box_pack_start(GTK_BOX(box), button, doexpand, dofill, padding);
  gtk_widget_show(button);
  itembox->next = button;


  gtk_tooltips_set_tip(curtrack, button,
		       xmms_remote_get_playlist_title
		       (session, xmms_remote_get_playlist_pos(session)), NULL);

  gtk_container_set_border_width(GTK_CONTAINER(box), 2);
  gtk_widget_show(box);
  gtk_container_add(GTK_CONTAINER(ctrl->base), box);

  ctrl->data = (gpointer)itembox;
  ctrl->with_popup = FALSE;
  
  return(TRUE);
}

static void xmms_plugin_free(Control *ctrl){
  
  items *itemcontainer;

  itemcontainer = g_new(items, 1);

  g_return_if_fail(ctrl != NULL);
  g_return_if_fail(ctrl-> data != NULL);

  itemcontainer = (items *)ctrl->data;
  g_free(itemcontainer);
}

static void xmms_plugin_read_config(Control *ctrl, xmlNodePtr parent){
  /* do something useful here...like...what? */
}

static void xmms_plugin_write_config(Control *ctrl, xmlNodePtr parent){
  /* do something useful here...er wait...
     we dont even read anything at startup, why write?! 
     and heck, i dont even remember how to use libxml!
  */
}

static void xmms_plugin_attach_callback(Control *ctrl, const gchar *signal, GCallback cb, gpointer data){
  
  items *mydata;

  mydata = g_new(items, 1);

  mydata = (items *)ctrl->data;
  
  g_signal_connect(mydata->prev, signal, cb, data);
  g_signal_connect(mydata->play, signal, cb, data);
  g_signal_connect(mydata->pause, signal, cb, data);
  g_signal_connect(mydata->stop, signal, cb, data);
  g_signal_connect(mydata->next, signal, cb, data);
}

static void xmms_plugin_set_size(Control *ctrl, int size){
  /* do the resize */
  /* yeah...do it....or something */
}

/* options dialog */
static void xmms_plugin_create_options (Control *ctrl, GtkContainer *con, GtkWidget *done){

}

/* initialization */
G_MODULE_EXPORT void xfce_control_class_init(ControlClass *cc){
  /* these are required */
  cc->name		= "xmms_plugin";
  cc->caption		= _("XMMS Control");
  
  cc->create_control	= (CreateControlFunc) xmms_plugin_control_new;

  cc->free		= xmms_plugin_free;
  cc->attach_callback	= xmms_plugin_attach_callback;
  
  /* options */
  cc->read_config		= xmms_plugin_read_config;
  cc->write_config	        = xmms_plugin_write_config;
  cc->create_options		= xmms_plugin_create_options;
  
  /* Don't use this function at all if you want xfce to
   * do the sizing.
   * Just define the set_size function to NULL, or rather, don't 
   * set it to something else.
   */
  cc->set_size		= xmms_plugin_set_size;
  
  /* unused in the sample:
   * ->set_orientation
   * ->set_theme
   */
}

/* required! defined in panel/plugins.h */
XFCE_PLUGIN_CHECK_INIT
