/*
 * Copyright (c) 2003 Patrick van Staveren <pvanstav@cs.wmich.edu>
 * Copyright (c) 2005 Kemal Ilgar Eroglu <kieroglu@math.washington.edu>
 * Copyright (c) 2005 Mario Streiber <mario.streiber@gmx.de>
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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#include <gtk/gtk.h>

#include <libxfce4util/i18n.h>
#include <libxfcegui4/dialogs.h>
#include <panel/plugins.h>
#include <panel/xfce.h>

#include <xmmsctrl.h>

#include "xmms_plugin.h"

/******************/
/* xmms_control.c */
/******************/

/**************************************/
/* displays or hides the xmms windows */
/**************************************/
static void display_xmms(plugin_data *pd, gboolean show) {
  /* do nothing if xmms is not running */
  if (!xmms_remote_is_running(pd->xmms_session)) return;

  if (show) {
    /* display xmms windows based on their prior status */
    if (pd->show_mw) xmms_remote_main_win_toggle(pd->xmms_session, TRUE);
    if (pd->show_pl) xmms_remote_pl_win_toggle  (pd->xmms_session, TRUE);
    if (pd->show_eq) xmms_remote_eq_win_toggle  (pd->xmms_session, TRUE);
    pd->xmmsvisible = TRUE;
  }
  else {
    /* save status of each xmms window */
    pd->show_mw = xmms_remote_is_main_win(pd->xmms_session);
    pd->show_pl = xmms_remote_is_pl_win  (pd->xmms_session);
    pd->show_eq = xmms_remote_is_eq_win  (pd->xmms_session);

    /* make sure at least the main window is shown on redisplay */
    if (!(pd->show_pl || pd->show_eq)) pd->show_mw = TRUE;

    /* hide all */
    xmms_remote_main_win_toggle(pd->xmms_session, FALSE);
    xmms_remote_pl_win_toggle  (pd->xmms_session, FALSE);
    xmms_remote_eq_win_toggle  (pd->xmms_session, FALSE);
    pd->xmmsvisible = FALSE;
  }
}


/**************************************/
/* function to do the size adjustment */
/**************************************/
static void adjust_size(plugin_data *pd) {

  if (pd->size_adjust) {
     gtk_widget_set_size_request (pd->pbar, 0, 4);
     gtk_widget_set_size_request (pd->viewport, 0, 6);
  }
  else {
     gtk_widget_set_size_request (pd->pbar, 0, -1);
     gtk_widget_set_size_request (pd->viewport, 0, -1);
  }

  gtk_widget_show_all(pd->boxMain);
  if( !pd->show_scrolledtitle )
     gtk_widget_hide_all(pd->viewport);
}


/*********************************************************/
/* callback to jump to the previous song in the playlist */
/*********************************************************/
static void prev(GtkWidget *widget, GdkEventButton* event, gpointer data) {
  if (event->button != 1) return;
  xmms_remote_playlist_prev(((plugin_data*) data)->xmms_session);
}


/************************************************************************/
/* callback to start playing; if xmms is not running it will be started */
/************************************************************************/
static void play(GtkWidget *widget, GdkEventButton* event, gpointer data) {
  plugin_data *pd = (plugin_data*) data;
  int i=0;
  pid_t pid;
  int status;

  if (event->button != 1) return;

  /* start xmms and hide its windows if not already running */
  if (!xmms_remote_is_running(pd->xmms_session)) {
    if (exec_command( (pd->use_bmp)? "beep-media-player -p" : "xmms -p")) {
      while (!xmms_remote_is_running(pd->xmms_session) && (i <= 5)) {
        sleep(1);
        i++;
      }
    }
    if ((i < 5) && (!pd->xmmsvisible))
      display_xmms(pd, FALSE);
  }
  else xmms_remote_play(pd->xmms_session);
}


/*****************************/
/* callback for pausing xmms */
/*****************************/
static void paus(GtkWidget *widget, GdkEventButton* event, gpointer data) {
  if (event->button != 1) return;
  xmms_remote_pause(((plugin_data*) data)->xmms_session);
}


/******************************/
/* callback for stopping xmms */
/******************************/
static void stop(GtkWidget *widget, GdkEventButton* event, gpointer data) {
  if (event->button != 1) return;
  xmms_remote_stop(((plugin_data*) data)->xmms_session);
}

/*****************************************************/
/* callback to jump to the next song in the playlist */
/*****************************************************/
static void next(GtkWidget *widget, GdkEventButton* event, gpointer data) {
  if (event->button != 1) return;
  xmms_remote_playlist_next(((plugin_data*) data)->xmms_session);
}


/****************************************************************/
/* callback for changing the volume when using the scroll wheel */
/****************************************************************/
static void box_scroll(GtkWidget* widget, GdkEventScroll* event, gpointer data) {
  gint vl, vr;
  plugin_data *pd = (plugin_data*) data;

  xmms_remote_get_volume(pd->xmms_session, &vl, &vr);
  if(event->direction == GDK_SCROLL_UP) xmms_remote_set_volume(pd->xmms_session, vl+8, vr+8);
  else                                  xmms_remote_set_volume(pd->xmms_session, vl-8, vr-8);
  
  xmms_remote_get_volume(pd->xmms_session, &vl, &vr);
  gtk_progress_bar_set_fraction     (GTK_PROGRESS_BAR(pd->vol_pbar), ((double)(MAX(vl,vr)))/100);
}

/**********************************************/
/* callback for clicking on the progress bar  */
/* button 1: jump to time in the current song */
/* button 2: toggle xmms window visibility    */
/**********************************************/
static void pbar_click(GtkWidget* widget, GdkEventButton* event, gpointer data) {
  gint width, time, total;
  plugin_data *pd = (plugin_data *) data;
  
  switch (event->button) {
  case 1:
    // jump to time within this song
    width = pd->pbar->allocation.width;
    total = xmms_remote_get_playlist_time(pd->xmms_session,
                                          xmms_remote_get_playlist_pos(pd->xmms_session));
    time  = (int) (event->x * total / width);
    xmms_remote_jump_to_time(pd->xmms_session, time);
    break;

  case 2:
    // toggle xmms window visibility
    display_xmms(pd, !pd->xmmsvisible);
  }
}


/*******************/
/* panel_display.c */
/*******************/


/************************************************/
/* Set song title in tooltip and scrolled label */
/************************************************/
void set_song_title(plugin_data *pd) {
  gchar    *title, *tooltip, *label;
  gint     pos, time;
  gboolean running = xmms_remote_is_running(pd->xmms_session);

  if (running) {
    pos        = xmms_remote_get_playlist_pos(pd->xmms_session);
    time       = xmms_remote_get_playlist_time(pd->xmms_session, pos) / 1000;
    title      = xmms_remote_get_playlist_title(pd->xmms_session, pos);
    if (pd->simple_title)
       tooltip = g_strdup_printf("%s", title);
    else
       tooltip = g_strdup_printf("%d: %s (%d:%02d)", pos, title, time/60, time%60);
  }
  else
       tooltip = g_strdup_printf(TITLE_STRING);
  gtk_tooltips_set_tip            (pd->tooltip, GTK_WIDGET(pd->base), tooltip, NULL);

  if (pd->simple_title) label   = g_strdup_printf("%s %s ", tooltip, tooltip);
  else                  label   = g_strdup_printf("%s +++ %s +++", tooltip, tooltip);
  pd->labelattr->start_index    = 0;
  pd->labelattr->end_index      = strlen(label);
  gtk_label_set_attributes(GTK_LABEL(pd->label), pd->labelattrlist);
  gtk_label_set_text(GTK_LABEL(pd->label), label);
  //g_free(title);
  g_free(tooltip);
  g_free(label);
}


/*************************************************/
/* timeout function to update the plugin widgets */
/*************************************************/
gboolean pbar_label_update(gpointer data) {
  plugin_data    *pd = (plugin_data*) data;
  GtkAdjustment  *adj;
  gint           sp, len, time = 1, pl_pos = -1, play_time = 0, vl, vr;
  gchar          *timestr;
  PangoAttribute *attr;
  gboolean       running, playing;

  /* check xmms status */
  running  = xmms_remote_is_running(pd->xmms_session);
  playing  = (running && xmms_remote_is_playing(pd->xmms_session));
  if (running) { /* get playlist position and song length */
    pl_pos = xmms_remote_get_playlist_pos(pd->xmms_session);
    time   = xmms_remote_get_playlist_time(pd->xmms_session, pl_pos) / 1000;
  }

  /* update tooltip and song title */
  if (pd->playlist_position != pl_pos) {
    pd->playlist_position     = pl_pos;
    pd->title_scroll_position = 0;
    set_song_title(pd);
  }

  /* update progress bar */
  if (playing) play_time = xmms_remote_get_output_time(pd->xmms_session) / 1000;
  if (pd->play_time != play_time) {
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pd->pbar), (gdouble) play_time / time);
    // timestr = g_strdup_printf("%d:%02d", play_time/60, play_time%60);
    // gtk_progress_bar_set_text(GTK_PROGRESS_BAR(pd->pbar), timestr);
    // g_free(timestr);
    pd->play_time = play_time;
  }

  /* scroll song title */
  if ((pd->show_scrolledtitle) && (pd->scroll_step > 0)) {
    adj = gtk_viewport_get_hadjustment(GTK_VIEWPORT(pd->viewport));
    len = pd->label->allocation.width / 2;
    sp  = pd->title_scroll_position - pd->step_delay;
    sp  = (sp < 0) ? 0 : sp;
    sp  = (len > 0) ? sp % len : 0;
    gtk_adjustment_set_value(adj, sp);
    pd->title_scroll_position += pd->scroll_step;
  }
  
  if((running) && (pd->vol_pbar_visible)){
    xmms_remote_get_volume(pd->xmms_session, &vl, &vr);
    gtk_progress_bar_set_fraction     (GTK_PROGRESS_BAR(pd->vol_pbar), ((double)(MAX(vl,vr)))/100);    
  }
  
  /* set up new timer and destroy old if new scroll speed has been set */
  if (pd->timer_reset) {
    g_source_remove(pd->timeout);
    pd->timeout = g_timeout_add(1000 / pd->scroll_speed, pbar_label_update, pd);
    pd->timer_reset = FALSE;
    return FALSE;
  }
  return TRUE;
}


/***************************************************************/
/* adds a new button with image and callback to the parent box */
/***************************************************************/
static void new_button_with_img(GtkWidget *box, gchar *filename, gpointer cb, gpointer data) {
  GtkWidget *image, *eventbox;

  eventbox            = gtk_event_box_new();
  image               = gtk_image_new_from_file(filename);
  gtk_widget_show       (image);
  gtk_container_add     (GTK_CONTAINER(eventbox), image);
  gtk_widget_set_events (eventbox, GDK_BUTTON_PRESS_MASK);
  g_signal_connect      (G_OBJECT(eventbox), "button_press_event", G_CALLBACK(cb), data);
  gtk_box_pack_start    (GTK_BOX(box), eventbox, DOEXPAND, DOFILL, PADDING);
}


/******************************/
/* creates the plugin widgets */
/******************************/
gboolean xmms_plugin_control_new(Control *ctrl) {
  GtkWidget *button, *box, *boxV, *boxMain, *pbar, *vol_pbar, *viewport, *eventbox, *label;
  plugin_data *pd;
  gchar *title = TITLE_STRING" +++ "TITLE_STRING" +++ ";
  gint vl, vr;
  GtkRcStyle *rc;
  GdkColor color;

  pd = g_new(plugin_data, 1);
  
  /* These defaults will be overwritten by read config */
  pd->titletextsize          = TITLE_SIZE;
  pd->title_scroll_position  = 0;
  pd->scroll_speed           = SCROLL_SPEED;
  pd->scroll_step            = SCROLL_STEP;
  pd->step_delay             = STEP_DELAY;
  pd->scroll_delay           = SCROLL_DELAY;
  pd->playlist_position      = -1;
  pd->play_time              = -1;
  pd->xmmsvisible            = TRUE;
  pd->xmms_session           = 0;
  pd->timeout                = 0;
  pd->timer_reset            = FALSE;
  pd->show_scrolledtitle     = TRUE;
  pd->tooltip                = gtk_tooltips_new();
  pd->labelattrlist          = pango_attr_list_new();
  pd->labelattr              = pango_attr_size_new(pd->titletextsize * PANGO_SCALE);
  pd->labelattr->start_index = 0;
  pd->labelattr->end_index   = strlen(title);
  pd->quit_xmms              = FALSE;
  pd->simple_title           = FALSE;
  pd->size_adjust            = FALSE;
  pd->pbar_visible           = TRUE;
  pd->vol_pbar_visible       = TRUE;
  pd->use_bmp                = FALSE;
  pango_attr_list_insert       (pd->labelattrlist, pd->labelattr);

  

  /* add scrolling callback for the plugin base widget */
  pd->base                     = ctrl->base;
  gtk_widget_add_events(GTK_WIDGET(ctrl->base), GDK_SCROLL_MASK);
  g_signal_connect(G_OBJECT(ctrl->base),"scroll_event",G_CALLBACK(box_scroll), pd);
  gtk_tooltips_set_tip           (pd->tooltip, GTK_WIDGET(ctrl->base), TITLE_STRING, NULL);

  /* main container for the plugin widgets */
  boxMain                      = gtk_hbox_new(FALSE, 0);
  boxV                         = gtk_vbox_new(FALSE, 0);

  /* label for the song title */
  eventbox                     = gtk_event_box_new();
  label                        = gtk_label_new(title);
  gtk_label_set_line_wrap        (GTK_LABEL(label), FALSE);
  gtk_container_add              (GTK_CONTAINER(eventbox), label);
  gtk_widget_set_events          (eventbox, GDK_BUTTON_PRESS_MASK);
  gtk_label_set_attributes       (GTK_LABEL(label), pd->labelattrlist);

  /* viewport widget that manages the scrolling */
  viewport                     = gtk_viewport_new(NULL, NULL);
  gtk_viewport_set_shadow_type   (GTK_VIEWPORT(viewport), GTK_SHADOW_NONE);
  gtk_container_add              (GTK_CONTAINER(viewport), eventbox);
  gtk_widget_set_size_request    (viewport, 0, -1);
  gtk_box_pack_start             (GTK_BOX(boxV), viewport, DOEXPAND, DOFILL, PADDING);

  /* the progress bar */
  pbar                         = gtk_progress_bar_new();
  gtk_progress_bar_set_bar_style (GTK_PROGRESS_BAR(pbar), GTK_PROGRESS_CONTINUOUS);
  gtk_widget_set_size_request    (pbar, 0, -1);
  gtk_widget_set_events          (pbar, GDK_BUTTON_PRESS_MASK);
  g_signal_connect               (G_OBJECT(pbar), "button_press_event",
                                  G_CALLBACK(pbar_click), pd);
  gtk_box_pack_start             (GTK_BOX(boxV), pbar, DOEXPAND, DOFILL, PADDING);
       
  pd->boxMain  = boxMain;
  pd->viewport = viewport;
  pd->label    = label;
  pd->pbar     = pbar;

  /* box that contains the xmms control buttons */
  box                          = gtk_hbox_new(FALSE, 0);
  new_button_with_img(box, PREV, G_CALLBACK(prev), pd);
  new_button_with_img(box, PLAY, G_CALLBACK(play), pd);
  new_button_with_img(box, PAUS, G_CALLBACK(paus), pd);
  new_button_with_img(box, STOP, G_CALLBACK(stop), pd);
  new_button_with_img(box, NEXT, G_CALLBACK(next), pd);

  gtk_box_pack_start             (GTK_BOX(boxV), box, DOEXPAND, DOFILL, PADDING);    
  gtk_container_set_border_width (GTK_CONTAINER(boxMain), 2);  
  
  /* the volume progress bar */
  vol_pbar                          = gtk_progress_bar_new();
  gtk_progress_bar_set_orientation  (GTK_PROGRESS_BAR(vol_pbar),GTK_PROGRESS_BOTTOM_TO_TOP);
  gtk_progress_bar_set_bar_style    (GTK_PROGRESS_BAR(vol_pbar), GTK_PROGRESS_CONTINUOUS);
  gtk_widget_set_size_request       (vol_pbar, 6, 0);
  xmms_remote_get_volume            (pd->xmms_session, &vl, &vr);
  gtk_progress_bar_set_fraction     (GTK_PROGRESS_BAR(vol_pbar), ((double)(MAX(vl, vr)))/100);
  rc =                              gtk_widget_get_modifier_style (GTK_WIDGET (vol_pbar));

  if (!rc)
	rc = gtk_rc_style_new ();

  gdk_color_parse ("#00c000", &color);

  if (rc) {
	rc->color_flags[GTK_STATE_PRELIGHT] |= GTK_RC_BG;
	rc->bg[GTK_STATE_PRELIGHT] = color;
  }

  gtk_widget_modify_style (GTK_WIDGET (vol_pbar), rc);
  pd->vol_pbar                      = vol_pbar;
  
  gtk_box_pack_start             (GTK_BOX(boxMain), boxV, FALSE, FALSE, 1);
  gtk_box_pack_start             (GTK_BOX(boxMain), vol_pbar, FALSE, FALSE, 1);
  
  
  gtk_container_add              (GTK_CONTAINER(ctrl->base), boxMain);

  gtk_widget_show_all(boxMain);

  pd->timeout                  = g_timeout_add(1000 / pd->scroll_speed, pbar_label_update, pd);
                                              
  ctrl->data = (gpointer) pd;
  ctrl->with_popup = FALSE;
 
  return(TRUE);
}


/******************************************************/
/* frees all resources used by the plugin (I hope :-) */
/******************************************************/
static void xmms_plugin_free(Control *ctrl) {
  plugin_data *pd;

  g_return_if_fail(ctrl != NULL);
  g_return_if_fail(ctrl-> data != NULL);

  pd = (plugin_data*) ctrl->data;

  /* remove timeout */
  if (pd->timeout) g_source_remove(pd->timeout);

  /* destroy all widgets */
  gtk_widget_destroy(pd->boxMain);

  /* destroy text attribute for the label widget */
  pango_attribute_destroy(pd->labelattr);

  /* destroy the tooltips */
  gtk_object_destroy(GTK_OBJECT(pd->tooltip));

  /* let xmms exit if quit_xmms option is active */
  if (pd->quit_xmms) xmms_remote_quit(pd->xmms_session);

  /* free the plugin data structure */
  g_free(pd);
}
     

/*****************/
/* preferences.c */
/*****************/


/**************************/
/* read plugin parameters */
/**************************/
void xmms_plugin_read_config(Control *control, xmlNodePtr node) {
  xmlChar *value;
  gint n;
  gboolean b;
  plugin_data     *pd = (plugin_data *) control->data;
  PangoAttrSize *attr = (PangoAttrSize*) pd->labelattr;

  g_return_if_fail(node!=NULL && node->children!=NULL);

  /* Read xml properties. All values out of range will be ignored. */
  /* The default will be used then.                                */
  node=node->children;

  /* get song text size */
  value = xmlGetProp (node, (const xmlChar *) "textsize");
  if (value) {
    n = atoi((char*) value);
    if ((n >= MIN_TITLE_SIZE) && (n <= MAX_TITLE_SIZE)) {
      pd->titletextsize = n;
      attr->size = pd->titletextsize * PANGO_SCALE;
      gtk_label_set_attributes(GTK_LABEL(pd->label), pd->labelattrlist);
    }
  }
  xmlFree(value);

  /* get scroll speed */
  value = xmlGetProp (node, (const xmlChar *) "scroll_speed");
  if (value) {
    n = atoi((char*) value);
    if ((n >= MIN_SCROLL_SPEED) && (n <= MAX_SCROLL_SPEED)) {
      pd->scroll_speed = n;
      pd->timer_reset = TRUE;
    }
  }
  xmlFree(value);

  /* get scroll step width */
  value = xmlGetProp (node, (const xmlChar *) "scroll_step");
  if (value) {
    n = atoi((char*) value);
    if ((n >= MIN_SCROLL_STEP) && (n <= MAX_SCROLL_STEP)) pd->scroll_step = n;
  }
  xmlFree(value);

  /* get scroll delay */
  value = xmlGetProp (node, (const xmlChar *) "scroll_delay");
  if (value) {
    n = atoi((char*) value);
    if ((n >= MIN_SCROLL_DELAY) && (n <= MAX_SCROLL_DELAY)) pd->scroll_delay = n;
  }
  xmlFree(value);
  pd->step_delay = pd->scroll_delay * pd->scroll_speed * pd->scroll_step;

  /* get xmms window visibility */
  value = xmlGetProp (node, (const xmlChar *) "xmms_visible");
  if (value) {
    if (!strcmp(value, "TRUE")) pd->xmmsvisible = TRUE;
    else                        pd->xmmsvisible = FALSE;
    xmlFree(value);
  }

  /* get visibility  of scrolled song title */
  value = xmlGetProp (node, (const xmlChar *) "title_visible");
  if (value) {
    if (!strcmp(value, "TRUE"))
       pd->show_scrolledtitle = TRUE;
    else {
       pd->show_scrolledtitle = FALSE;
       gtk_widget_hide_all (pd->viewport);
    }
    xmlFree(value);
  }
  /* show/hide the event box parent of the scrolled title */
  if (pd->show_scrolledtitle)   gtk_widget_show_all(gtk_widget_get_parent(pd->label));
  else                          gtk_widget_hide_all(gtk_widget_get_parent(pd->label));

  /* load quit xmms option */
  value = xmlGetProp (node, (const xmlChar *) "quit_xmms");
  if (value) {
     if (!strcmp(value, "TRUE")) pd->quit_xmms = TRUE;
     else                        pd->quit_xmms = FALSE;
     xmlFree(value);
  }

  /* load simple title option */
  value = xmlGetProp (node, (const xmlChar *) "simple_title");
  if (value) {
     if (!strcmp(value, "TRUE")) pd->simple_title = TRUE;
     else                        pd->simple_title = FALSE;
     xmlFree(value);
  }
  set_song_title(pd);

  /* load size adjust option */
  value = xmlGetProp (node, (const xmlChar *) "size_adjust");
  if (value) {
     if (!strcmp(value, "TRUE")) pd->size_adjust = TRUE;
     else                        pd->size_adjust = FALSE;
     xmlFree(value);
  }
  adjust_size(pd);
  
  /* load progressbar visibility option */
  value = xmlGetProp (node, (const xmlChar *) "pbar_visible");
  if (value) {
     if (!strcmp(value, "TRUE")) {
           pd->pbar_visible = TRUE;
           gtk_widget_show_all  (pd->pbar);
     }
     else {                 
           pd->pbar_visible = FALSE;
           gtk_widget_hide_all  (pd->pbar);
     }
           
     xmlFree(value);
  }

  /* load volume progressbar visibility option */
  value = xmlGetProp (node, (const xmlChar *) "vol_pbar_visible");
  if (value) {
     if (!strcmp(value, "TRUE")) {
           pd->vol_pbar_visible = TRUE;
           gtk_widget_show_all  (pd->vol_pbar);
     }
     else {                 
           pd->vol_pbar_visible = FALSE;
           gtk_widget_hide_all  (pd->vol_pbar);
     }
           
     xmlFree(value);
  }

  /* load use bmp option */
  value = xmlGetProp (node, (const xmlChar *) "use_bmp");
  if (value) {
     if (!strcmp(value, "TRUE"))
           pd->use_bmp = TRUE;
     else
           pd->use_bmp = FALSE;
           
     xmlFree(value);
  }
}


/**************************/
/* save plugin parameters */
/**************************/
void xmms_plugin_write_config(Control *ctrl, xmlNodePtr parent) {
  xmlChar value[10];
  plugin_data *pd=(plugin_data *)ctrl->data;
  xmlNodePtr node;

  g_return_if_fail(parent!=NULL);
         
  node = xmlNewTextChild (parent, NULL, (const xmlChar *)"XmmsControl", NULL);
  g_return_if_fail(node!=NULL);
       
  /* save title font size */
  g_snprintf (value, 5, "%d", pd->titletextsize);
  xmlSetProp (node, "textsize", (const xmlChar *)value);

  /* save scroll speed */
  g_snprintf (value, 5, "%d", pd->scroll_speed);
  xmlSetProp (node, "scroll_speed", (const xmlChar *)value);

  /* save scroll step width */
  g_snprintf (value, 5, "%d", pd->scroll_step);
  xmlSetProp (node, "scroll_step", (const xmlChar *)value);

  /* save scroll delay */
  g_snprintf (value, 5, "%d", pd->scroll_delay);
  xmlSetProp (node, "scroll_delay", (const xmlChar *)value);

  /* save xmms window visibility */
  xmlSetProp (node, "xmms_visible", (pd->xmmsvisible) ? "TRUE" : "FALSE");

  /* save visibility of scrolled sing title */
  xmlSetProp (node, "title_visible", (pd->show_scrolledtitle) ? "TRUE" : "FALSE");

  /* save quit xmms option */
  xmlSetProp (node, "quit_xmms", (pd->quit_xmms) ? "TRUE" : "FALSE");

  /* save simple title option */
  xmlSetProp (node, "simple_title", (pd->simple_title) ? "TRUE" : "FALSE");

  /* save size adjust option */
  xmlSetProp (node, "size_adjust", (pd->size_adjust) ? "TRUE" : "FALSE");
  
  /* save progressbar visibility option */
  xmlSetProp (node, "pbar_visible", (pd->pbar_visible) ? "TRUE" : "FALSE");  
  
  /* save volume progressbar visibility option */
  xmlSetProp (node, "vol_pbar_visible", (pd->vol_pbar_visible) ? "TRUE" : "FALSE");  

  /* save use bmp option */
  xmlSetProp (node, "use_bmp", (pd->use_bmp) ? "TRUE" : "FALSE");    
}


/**************************************************************/
/* generic function to add panel callbacks to the base widget */
/**************************************************************/
static void xmms_plugin_attach_callback(Control *ctrl, const gchar *signal, GCallback cb, gpointer data) {
  g_signal_connect(ctrl->base, signal, cb, data); 
}


/**************************************/
/* callback to change the plugin size */
/**************************************/
static void xmms_plugin_set_size(Control *ctrl, int size){
  /* do the resize */
  /* yeah...do it....or something */
}


/************************************/
/* callback to change the font size */
/************************************/
static void set_font_size(GtkSpinButton *spin, gpointer data) {
  plugin_data *pd     = (plugin_data*) data;
  PangoAttrSize *attr = (PangoAttrSize*) pd->labelattr;
  pd->titletextsize   = gtk_spin_button_get_value_as_int(spin);

  if (pd->titletextsize < MIN_TITLE_SIZE) pd->titletextsize = 1;
  if (pd->titletextsize > MAX_TITLE_SIZE) pd->titletextsize = MAX_TITLE_SIZE;
  attr->size = pd->titletextsize * PANGO_SCALE;
  gtk_label_set_attributes(GTK_LABEL(pd->label), pd->labelattrlist);
}


/***************************************/
/* callback to change the scroll speed */
/***************************************/
static void set_scroll_speed(GtkSpinButton *spin, gpointer data) {
  plugin_data *pd  = (plugin_data*) data;
  pd->scroll_speed = gtk_spin_button_get_value_as_int(spin);

  if (pd->scroll_speed < MIN_SCROLL_SPEED) pd->scroll_speed = MIN_SCROLL_SPEED;
  if (pd->scroll_speed > MAX_SCROLL_SPEED) pd->scroll_speed = MAX_SCROLL_SPEED;
  pd->timer_reset = TRUE;
}


/********************************************/
/* callback to change the scroll step width */
/********************************************/
static void set_scroll_step(GtkSpinButton *spin, gpointer data) {
  plugin_data *pd             = (plugin_data*) data;
  gint s                      = gtk_spin_button_get_value_as_int(spin);

  if (s < MIN_SCROLL_STEP) s  = MIN_SCROLL_STEP;
  if (s > MAX_SCROLL_STEP) s  = MAX_SCROLL_STEP;
  pd->scroll_step = s;
  if (s == 0) pd->title_scroll_position = 0;
}


/***************************************/
/* callback to change the scroll delay */
/***************************************/
static void set_scroll_delay(GtkSpinButton *spin, gpointer data) {
  plugin_data *pd             = (plugin_data*) data;
  gint d                      = gtk_spin_button_get_value_as_int(spin);
  
  if (d < MIN_SCROLL_DELAY) d = MIN_SCROLL_DELAY;
  if (d > MAX_SCROLL_DELAY) d = MAX_SCROLL_DELAY;
  pd->scroll_delay            = d;
  pd->step_delay              = d * pd->scroll_speed * pd->scroll_step;
  pd->title_scroll_position   = 0;
}


/***************************************************************/
/* callback to toggle the visibility of the scrolled songtitle */
/***************************************************************/
static void show_title(GtkToggleButton *check, gpointer data) {
  plugin_data *pd             = (plugin_data*) data;
  pd->show_scrolledtitle      = gtk_toggle_button_get_active(check);

  /* show/hide the event box parent of the scrolled title */
  if (pd->show_scrolledtitle)   gtk_widget_show_all(pd->viewport);
  else                          gtk_widget_hide_all(pd->viewport);
}


/***************************************/
/* callback to change quit xmms option */
/***************************************/
static void quit_xmms_toggled(GtkToggleButton *button, gpointer data) {
  plugin_data *pd = (plugin_data*) data;
  pd->quit_xmms   = gtk_toggle_button_get_active(button);
}


/******************************************/
/* callback to change simple title option */
/******************************************/
static void simple_title_toggled(GtkToggleButton *button, gpointer data) {
  plugin_data *pd    = (plugin_data*) data;
  pd->simple_title   = gtk_toggle_button_get_active(button);
  set_song_title(pd);
}


/**********************************/
/* callback to size adjust option */
/**********************************/
static void size_adjust_toggled(GtkToggleButton *button, gpointer data) {
  plugin_data *pd    = (plugin_data*) data;
  pd->size_adjust   = gtk_toggle_button_get_active(button);
  adjust_size(pd);
}

/*********************************************************/
/* callback for show track postion (pbar_visible) option */
/*********************************************************/
static void pbar_visible_toggled(GtkToggleButton *button, gpointer data) {
  plugin_data *pd   = (plugin_data*) data;
  pd->pbar_visible  = gtk_toggle_button_get_active(button);
  
  if(pd->pbar_visible)
     gtk_widget_show_all (pd->pbar);
  else
     gtk_widget_hide_all (pd->pbar);
}

/******************************************************/
/* callback for show volume (vol_pbar_visible) option */
/******************************************************/
static void vol_pbar_visible_toggled(GtkToggleButton *button, gpointer data) {
  plugin_data *pd        = (plugin_data*) data;
  pd->vol_pbar_visible   = gtk_toggle_button_get_active(button);
  
  if(pd->vol_pbar_visible)
     gtk_widget_show_all (pd->vol_pbar);
  else
     gtk_widget_hide_all (pd->vol_pbar);
}

/*********************************/
/* callback for "use bmp" option */
/*********************************/
static void use_bmp_toggled(GtkToggleButton *button, gpointer data) {
  plugin_data *pd  = (plugin_data*) data;
  pd->use_bmp      = gtk_toggle_button_get_active(button);

}

/*****************************************************/
/* add label to the left column of the options table */
/*****************************************************/
static void add_label(GtkWidget *table, gchar *text, gint pos) {
  gint att_opts               = GTK_SHRINK | GTK_EXPAND | GTK_FILL;
  GtkWidget *label            = gtk_label_new(text);

  gtk_misc_set_alignment        (GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach              (GTK_TABLE(table), label, 0, 1, pos, pos+1, att_opts, att_opts, 0, 0);
}


/*************************************************************/
/* add spin button to the right columns of the options table */
/*************************************************************/
static void add_spin(GtkWidget *table, gint min, gint max, gint value, GCallback cb, gpointer data, gint pos) {
  gint att_opts               = GTK_SHRINK/* | GTK_EXPAND | GTK_FILL*/;
  GtkWidget *spin             = gtk_spin_button_new_with_range(min, max, 1.0);

  gtk_spin_button_set_value     (GTK_SPIN_BUTTON(spin), value);
  gtk_table_attach              (GTK_TABLE(table), spin, 1, 2, pos, pos+1, att_opts, att_opts, 0, 0);
  g_signal_connect              (G_OBJECT(spin), "value-changed", cb, data);
}

/********************/
/* add check button */
/********************/
static void add_check(GtkWidget *parent, gchar *title, gboolean active, GCallback cb, gpointer data, const char* tip_title) {
  GtkWidget *check            = gtk_check_button_new_with_label(title);

  gtk_toggle_button_set_active  (GTK_TOGGLE_BUTTON(check), active);
  gtk_box_pack_start            (GTK_BOX(parent), check, DOEXPAND, DOFILL, PADDING);
  if(tip_title)
  	gtk_tooltips_set_tip    (((plugin_data *) data)->tooltip, check, tip_title, NULL);
  g_signal_connect              (G_OBJECT(check), "toggled", cb, data);
}

/**********************/
/* preferences dialog */
/**********************/
static void xmms_plugin_create_options (Control *ctrl, GtkContainer *con, GtkWidget *done) {
  plugin_data *pd = (plugin_data*) ctrl->data;
  GtkWidget *vbox, *table, *label, *size, *speed, *step, *delay;
  gint att_opts = GTK_SHRINK | GTK_EXPAND | GTK_FILL;

  gtk_tooltips_disable(pd->tooltip);
  gtk_tooltips_enable(pd->tooltip);

  vbox  = gtk_vbox_new      (FALSE, 4);
  table = gtk_table_new     (4, 2, FALSE);
  gtk_box_pack_start (GTK_BOX(vbox), gtk_hseparator_new(), DOEXPAND, DOFILL, PADDING);

  /* add table */
  gtk_box_pack_start        (GTK_BOX(vbox), table, DOEXPAND, DOFILL, PADDING);

  /* put labels into the left column of our table */
  add_label(table, "Font Size",        0);
  add_label(table, "Scroll Speed",     1);
  add_label(table, "Scroll Stepwidth", 2);
  add_label(table, "Scroll Delay",     3);

  /* put spin buttons into the right column */
  add_spin(table, MIN_TITLE_SIZE,   MAX_TITLE_SIZE,   pd->titletextsize, G_CALLBACK(set_font_size),    pd, 0);
  add_spin(table, MIN_SCROLL_SPEED, MAX_SCROLL_SPEED, pd->scroll_speed,  G_CALLBACK(set_scroll_speed), pd, 1);
  add_spin(table, MIN_SCROLL_STEP,  MAX_SCROLL_STEP,  pd->scroll_step,   G_CALLBACK(set_scroll_step),  pd, 2);
  add_spin(table, MIN_SCROLL_DELAY, MAX_SCROLL_DELAY, pd->scroll_delay,  G_CALLBACK(set_scroll_delay), pd, 3);

  /*new separator*/
  gtk_box_pack_start (GTK_BOX(vbox), gtk_hseparator_new(), DOEXPAND, DOFILL, PADDING);

  /* add check buttons for the scrolled title, progressbar and volume bar */
  add_check(vbox, "Show scrolled song title", pd->show_scrolledtitle, G_CALLBACK(show_title), pd, NULL);
  add_check(vbox, "Show track position", pd->pbar_visible, G_CALLBACK(pbar_visible_toggled), pd, NULL);
  add_check(vbox, "Show volume level", pd->vol_pbar_visible, G_CALLBACK(vol_pbar_visible_toggled), pd, NULL);
  /* add check button for simple title option */
  add_check(vbox, "Simple song title format", pd->simple_title, G_CALLBACK(simple_title_toggled), pd, NULL);
  /* add check button for size adjustment */
  add_check(vbox, "Size adjustment", pd->size_adjust, G_CALLBACK(size_adjust_toggled), pd, "Checking ths option can be useful if the panel looks larger than it should");
  /* add check button for "Use BMP" option */
  add_check(vbox, "Use BMP (Beep Media Player)", pd->use_bmp, G_CALLBACK(use_bmp_toggled), pd, NULL);  
  /* add check button for quit xmms option */
  add_check(vbox, "Quit XMMS/BMP when plugin terminates", pd->quit_xmms, G_CALLBACK(quit_xmms_toggled), pd, NULL);
  
  gtk_container_add  (GTK_CONTAINER(con), vbox);
  gtk_widget_show_all(vbox);
  
}

/*******************/
/* panel_display.c */
/*******************/


/******************/
/* initialization */
/******************/
G_MODULE_EXPORT void xfce_control_class_init(ControlClass *cc){
  /* these are required */
  cc->name		= "xmms_plugin";
  cc->caption		= _("XMMS Control");
  
  cc->create_control	= (CreateControlFunc) xmms_plugin_control_new;

  cc->free		= xmms_plugin_free;
  cc->attach_callback	= xmms_plugin_attach_callback;
  
  /* options */
  cc->read_config       = xmms_plugin_read_config;
  cc->write_config      = xmms_plugin_write_config;
  cc->create_options    = xmms_plugin_create_options;
  
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
