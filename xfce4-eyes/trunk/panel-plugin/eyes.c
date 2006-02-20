/*
 * Copyright (c) 2003 Benedikt Meurer <benedikt.meurer@unix-ag.uni-siegen.de>
 *                    Danny Milosavljevic <danny_milo@gmx.net>
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

/*
 * geyes.c - A cheap xeyes ripoff.
 * Copyright (C) 1999 Dave Camp
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_MATH_H
#include <math.h>
#endif

#include <gtk/gtk.h>

#include <libxfce4util/i18n.h>
#include <libxfcegui4/dialogs.h>
#include <panel/plugins.h>
#include <panel/xfce.h>

#include <sys/types.h>
#include <dirent.h>

#include "eyes.h"
#include "themes.h"

/* for xml: */
#define EYES_ROOT "Eyes"

#define DEFAULTTHEME	"Default-tiny"

extern xmlDocPtr xmlconfig;
#define MYDATA(node) xmlNodeListGetString(xmlconfig, node->children, 1)

#define UPDATE_TIMEOUT 100
/*75*/

/* TODO - Optimize this a bit */
static void calculate_pupil_xy (t_eyes *eyes_applet,
		    gint x, gint y,
		    gint *pupil_x, gint *pupil_y)
{
        double angle;
        double sina;
        double cosa;
        double nx;
        double ny;
        double h;
        
        nx = x - (double)eyes_applet->eye_width / 2;
        ny = y - (double)eyes_applet->eye_height / 2;
        
        angle = atan2(nx, ny);
        h = hypot(nx, ny);
	
        if (abs(h) < (abs(hypot(eyes_applet->eye_height / 2,
	    eyes_applet->eye_width / 2)) - eyes_applet->wall_thickness -
				eyes_applet->pupil_height)) {
                *pupil_x = x;
                *pupil_y = y;
                return;
        }
        
        sina = sin(angle);
        cosa = cos(angle);
        
        *pupil_x = hypot((eyes_applet->eye_height / 2) * cosa,
			(eyes_applet->eye_width / 2)* sina) * sina;
        *pupil_y = hypot((eyes_applet->eye_height / 2) * cosa,
			(eyes_applet->eye_width / 2)* sina) * cosa;
        *pupil_x -= hypot((eyes_applet->pupil_width / 2) * sina,
			(eyes_applet->pupil_height / 2)* cosa) * sina;
        *pupil_y -= hypot((eyes_applet->pupil_width / 2) * sina,
			(eyes_applet->pupil_height / 2) * cosa) * cosa;
        *pupil_x -= hypot((eyes_applet->wall_thickness / 2) * sina,
			(eyes_applet->wall_thickness / 2) * cosa) * sina;
        *pupil_y -= hypot((eyes_applet->wall_thickness / 2) * sina,
			(eyes_applet->wall_thickness / 2)* cosa) * cosa;
        
        *pupil_x += eyes_applet->eye_width / 2;
        *pupil_y += eyes_applet->eye_height / 2;
}

static void draw_eye (t_eyes *eyes_applet,
	  gint eye_num, 
          gint pupil_x, 
          gint pupil_y)
{
	/*
	 * XXX - gdk_pixbuf_render_to_drawable_alpha is deprecated. Any
	 * replacements? 
	 *
	 * dannym: yes ;)
	 */
	 
#if defined (GTK_CHECK_VERSION) && GTK_CHECK_VERSION(2,2,0)
	gdk_draw_pixbuf (eyes_applet->eyes[eye_num]->window, 
				/*gc*/NULL, 
				eyes_applet->eye_image, 
				0, 0, 
				0, 0, 
				eyes_applet->eye_width,
				eyes_applet->eye_height,
				GDK_RGB_DITHER_NONE,
				0, 0);
	/* how to do the alpha channel? */

	gdk_draw_pixbuf (
				     eyes_applet->eyes[eye_num]->window,
				     /*gc*/ NULL,
				     eyes_applet->pupil_image, 
				     0, 0, 
                         	     pupil_x - eyes_applet->pupil_width / 2, 
                         	     pupil_y - eyes_applet->pupil_height / 2,
                         	     -1, -1,
                         	     GDK_RGB_DITHER_NONE,
                         	     0,
                         	     0);

#else
	/*pixbuf,drwable(dest),src_x,src_y,dest_x,dest_y,w,h,resv,resv,dither,xdither,ydither) */
	gdk_pixbuf_render_to_drawable_alpha (eyes_applet->eye_image, 
				     eyes_applet->eyes[eye_num]->window,
				     0, 0, 
                         	     0, 0,
                         	     eyes_applet->eye_width, 
                         	     eyes_applet->eye_height,
                         	     GDK_PIXBUF_ALPHA_BILEVEL,
                         	     128,
                         	     GDK_RGB_DITHER_NONE,
                         	     0,
                         	     0);
        
	gdk_pixbuf_render_to_drawable_alpha (eyes_applet->pupil_image, 
				     eyes_applet->eyes[eye_num]->window,
				     0, 0, 
                         	     pupil_x - eyes_applet->pupil_width / 2, 
                         	     pupil_y - eyes_applet->pupil_height / 2,
                         	     -1, -1,
                         	     GDK_PIXBUF_ALPHA_BILEVEL,
                         	     128,
                         	     GDK_RGB_DITHER_NONE,
                         	     0,
                         	     0);
#endif
 
}

static gint timer_cb(t_eyes *eyes)
{
        gint x, y;
        gint pupil_x, pupil_y;
        gint i;
        
        for (i = 0; i < eyes->num_eyes; i++) {
		if (GTK_WIDGET_REALIZED(eyes->eyes[i])) {
			gdk_window_get_pointer(eyes->eyes[i]->window, &x, &y,
					NULL);
			calculate_pupil_xy(eyes, x, y, &pupil_x, &pupil_y);
			draw_eye(eyes, i, pupil_x, pupil_y);
		}
        }
        
        return(TRUE);
}

static void properties_load(t_eyes *eyes)
{
        gchar *path;

	if (eyes->options.theme) {
		path = g_build_filename(THEMESDIR, eyes->options.theme, NULL);
	}
	else {
		path = g_build_filename(THEMESDIR, DEFAULTTHEME, NULL);
	}
	
        load_theme(eyes, path);

        g_free(path);
}

static void setup_eyes(t_eyes *eyes) 
{
	int i;

	if (eyes->hbox != NULL) {
		gtk_widget_destroy(eyes->hbox);
		eyes->hbox = NULL;
	}

        eyes->hbox = gtk_hbox_new(FALSE, 0);

	gtk_container_add(GTK_CONTAINER(eyes->align), GTK_WIDGET(eyes->hbox));

        for (i = 0; i < eyes->num_eyes; i++) {
                eyes->eyes[i] = gtk_drawing_area_new();
              
		gtk_widget_set_size_request(GTK_WIDGET(eyes->eyes[i]),
					     eyes->eye_width,
					     eyes->eye_height);
 
                gtk_widget_show(eyes->eyes[i]);
                
		gtk_box_pack_start(GTK_BOX(eyes->hbox), eyes->eyes [i],
                                   FALSE, FALSE, 0);

		if (gtk_widget_get_parent_window(eyes->eyes[i])) {
	                gtk_widget_realize(eyes->eyes[i]);

			draw_eye(eyes, i,eyes->eye_width / 2,
					eyes->eye_height / 2);
	       }
        }

        gtk_widget_show(eyes->hbox);
}


static gboolean eyes_applet_fill(t_eyes *eyes)
{
	gtk_widget_show_all(GTK_WIDGET(eyes->align));

	if (eyes->timeout_id == 0) {
	        eyes->timeout_id = g_timeout_add (UPDATE_TIMEOUT,
				(GtkFunction)timer_cb, eyes);
	}

	return(TRUE);
}



static gboolean
eyes_control_new(Control *ctrl)
{
	t_eyes *eyes;

	xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
	
	eyes = g_new0(t_eyes, 1);

	eyes->ebox = gtk_event_box_new ();
	gtk_widget_show(GTK_WIDGET(eyes->ebox));
	
	eyes->align = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	
#if 0 /* vertical, old style */
	eyes->align  = gtk_alignment_new (0.0, 0.5, 1.0, 0.0);
#endif

	gtk_widget_show(GTK_WIDGET(eyes->align));
	
	gtk_container_add(GTK_CONTAINER(eyes->ebox), GTK_WIDGET(eyes->align));

	gtk_container_add(GTK_CONTAINER(ctrl->base), GTK_WIDGET(eyes->ebox));

	ctrl->data = (gpointer)eyes;
	ctrl->with_popup = FALSE;

        properties_load(eyes);
        setup_eyes(eyes);
	eyes_applet_fill(eyes);

	return(TRUE);
}

static void
eyes_free_data(XfcePanelPlugin* plugin, t_eyes* eyes)
{
 /* TODO free property dialog? */
 
	g_return_if_fail(plugin != NULL);
	g_return_if_fail(eyes != NULL);

	if (eyes->timeout_id != 0)
		g_source_remove(eyes->timeout_id);

	if (eyes->options.theme != NULL)
		g_free(eyes->options.theme);
	
	if (eyes->eye_image != NULL)
		g_object_unref(eyes->eye_image);
	
	if (eyes->pupil_image != NULL)
		g_object_unref(eyes->pupil_image);
	
	if (eyes->theme_dir != NULL)
		g_free(eyes->theme_dir);

	if (eyes->theme_name != NULL)
		g_free(eyes->theme_name);

	if (eyes->eye_filename != NULL)
		g_free(eyes->eye_filename);

	if (eyes->pupil_filename != NULL)
		g_free(eyes->pupil_filename);
	
	gtk_widget_destroy(eyes->align);
	g_free(eyes);
}

static void
theme_changed_cb(GtkOptionMenu *om, t_eyes *eyes)
{
	GtkLabel *label;

	label = GTK_LABEL(gtk_bin_get_child (GTK_BIN (om)));
	
	if (eyes->options.theme)
		g_free(eyes->options.theme);

	eyes->options.theme = g_strdup(gtk_label_get_text(label));

	properties_load(eyes);
        setup_eyes (eyes);
	eyes_applet_fill(eyes);
}

static void
menu_add_string(GtkMenu *menu, gchar *text)
{
	GtkWidget *widget;

	widget = GTK_WIDGET(gtk_menu_item_new_with_label(text));
	gtk_widget_show(widget);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), widget);
}

static void
eyes_create_options(Control *control, GtkContainer *container, 
		GtkWidget *done)
{
	const gchar	*entry;
	t_eyes 		*eyes;
	GtkBox		*vbox;
	GtkMenu 	*m; /* the menu of the option menu */
	int		sel; /* "selected" index in the option menu */
	int		i; /* current index in the option menu */
	char		*current; /* currently used theme */
	GDir		*dir;
	
	eyes = (t_eyes *)control->data;
	
	eyes->dialog = gtk_widget_get_toplevel(done);
	
/*	if (eyes->revert.theme != NULL)
		g_free(eyes->revert.theme);
	
	if (eyes->options.theme != NULL)
		eyes->revert.theme = g_strdup(eyes->options.theme);
	else
		eyes->revert.theme = NULL;
*/
	current = (eyes->options.theme) ? eyes->options.theme : DEFAULTTHEME;

	m = GTK_MENU(gtk_menu_new());

	sel = 0;

	if ((dir = g_dir_open(THEMESDIR, 0, NULL)) == NULL) {
		/*
		 * fall back to default theme
		 */
		menu_add_string(m, g_strdup(DEFAULTTHEME));
	}
	else {
		for (i = 0; (entry = g_dir_read_name(dir)) != NULL; i++) {
			menu_add_string(m, g_strdup(entry));

			if (strcmp(entry, current) == 0) {
					sel = i;
			}
		}

		g_dir_close(dir);
	}
	
	
	vbox = GTK_BOX(gtk_vbox_new(FALSE, 5));
	gtk_widget_show(GTK_WIDGET(vbox));

	gtk_container_add(GTK_CONTAINER(container), GTK_WIDGET(vbox));	
	
	eyes->theme_om = GTK_WIDGET(gtk_option_menu_new());
	
	gtk_option_menu_set_menu(GTK_OPTION_MENU(eyes->theme_om),GTK_WIDGET(m));
	
	gtk_widget_show(eyes->theme_om);

	gtk_option_menu_set_history(GTK_OPTION_MENU(eyes->theme_om), sel);

	gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(eyes->theme_om), FALSE,
			FALSE, 0);
	
	g_signal_connect(GTK_WIDGET(eyes->theme_om), "changed",
			G_CALLBACK(theme_changed_cb), eyes);
}

static gboolean
eyes_set_size (XfcePanelPlugin *plugin, int size)
{
    if (xfce_panel_plugin_get_orientation (plugin) ==
            GTK_ORIENTATION_HORIZONTAL)
    {
        gtk_widget_set_size_request (GTK_WIDGET (plugin),
                                     -1, size);
    }
    else
    {
        gtk_widget_set_size_request (GTK_WIDGET (plugin),
                                     size, -1);
    }
 
    return TRUE;
}

static void
eyes_orientation_changed (XfcePanelPlugin *plugin, 
                           GtkOrientation orientation, 
                           t_eyes* eyes)
{
  if (orientation == GTK_ORIENTATION_VERTICAL) {
    gtk_alignment_set (GTK_ALIGNMENT (eyes->align), 0.5, 0.0, 0.0, 1.0);
  } else {
    gtk_alignment_set (GTK_ALIGNMENT (eyes->align), 0.0, 0.5, 1.0, 0.0);
  }
}

static void
eyes_read_rc_file (XfcePanelPlugin *plugin, t_eyes* eyes)
{
  XfceRc* rc;
  gchar const* theme1;

  if (eyes->options.theme != NULL) {
    g_free (eyes->options.theme);
    eyes->options.theme = NULL;
  }

  if ((file = xfce_panel_plugin_lookup_rc_file (plugin)) != NULL) {
    rc = xfce_rc_simple_open (file, TRUE);
    g_free (file);
    
    if (rc != NULL) {
      theme1 = xfce_rc_read_entry (rc, "theme", NULL);
      if (theme1 != NULL) {
        eyes->options.theme = g_strdup (theme1);
      }
    }
  }
}

static void
eyes_write_rc_file (XfcePanelPlugin *plugin, t_eyes* eyes)
{
    char *file;
    XfceRc *rc;
    
    if (!(file = xfce_panel_plugin_save_location (plugin, TRUE)))
        return;

    rc = xfce_rc_simple_open (file, FALSE);
    g_free (file);

    if (!rc)
        return;
 
    if (eyes->options.theme != NULL) {
      xfce_rc_write_entry (rc, "theme", eyes->options.theme);
    }

    xfce_rc_close (rc);
}

                                                      
static void eyes_construct (XfcePanelPlugin *plugin);

XFCE_PANEL_PLUGIN_REGISTER_INTERNAL(eyes_construct);

static void 
eyes_construct (XfcePanelPlugin *plugin)
{
    t_eyes *eyes = g_new0 (t_eyes, 1);

    g_signal_connect (plugin, "orientation-changed", 
                      G_CALLBACK (eyes_orientation_changed), eyes);
    
    g_signal_connect (plugin, "size-changed", 
                      G_CALLBACK (eyes_set_size), NULL);
    
    g_signal_connect (plugin, "free-data", 
                      G_CALLBACK (eyes_free_data), eyes);
    
    g_signal_connect (plugin, "save", 
                      G_CALLBACK (eyes_write_rc_file), eyes);

    xfce_panel_plugin_menu_show_configure (plugin);
    g_signal_connect (plugin, "configure-plugin", 
                      G_CALLBACK (eyes_properties_dialog), eyes);

    eyes->plugin = plugin;

    eyes_read_rc_file (plugin, eyes);

    gtk_container_add (GTK_CONTAINER (plugin), eyes->eyes);
    
    xfce_panel_plugin_add_action_widget (plugin, eyes->eyes);
}
