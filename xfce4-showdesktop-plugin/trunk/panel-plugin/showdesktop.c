// Copyright {{{ 

/*
 * Copyright (c) 2003 Andre Lerche <a.lerche@gmx.net>
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

// }}}

// some includes {{{

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>

#include <libxfce4util/i18n.h>
#include <panel/plugins.h>
#include <panel/xfce.h>
#include <libxfcegui4/netk-screen.h>

#define HORIZONTAL 0
#define VERTICAL 1

// }}}

// 2 structs {{{

typedef struct
{
    const char *signal;
    GCallback callback;
    gpointer data;
}
SignalCallback;

typedef struct
{
    GtkWidget	*show_all;
    GtkWidget	*hide_all;
    GtkWidget   *show_image;
    GtkWidget   *hide_image;
    GtkWidget   *box;
    GtkWidget	*ebox;
    int         orientation;
    gboolean    swapCommands;
    gboolean    showTooltips;
    gboolean    lessSpace;
    SignalCallback *cb;
} gui;

// }}}

// all functions {{{

         /* 
	    do_window_actions: minimize or unminimize all unsticky windows on
	    the current workspace
	 */

int
do_window_actions (int type)
{
    NetkScreen *screen;
    NetkWorkspace *workspace;
    NetkWindow *window;
    NetkWindow *active_window = NULL;
    GList *w;
    
    screen = netk_screen_get (0);
    workspace = netk_screen_get_active_workspace (screen);
    w = netk_screen_get_windows_stacked (screen);
   
    while (w != NULL) {
        window = w->data;
        if (!netk_window_is_sticky (window)) {
            if (workspace == netk_window_get_workspace (window)) {
		active_window = window;
                if (type == 0) {
	            netk_window_minimize (window);
	        } else {
		    if (netk_window_is_minimized (window)) {
	                netk_window_unminimize (window);
	            }
	        }
	    }
	 }
 	 w = w->next;   
    }
    if (active_window != NULL && type == 1) {
        netk_window_activate (active_window);
    }
}

SignalCallback *
track_callback (const char *signal, GCallback callback, gpointer data, gui *plugin)
{
    SignalCallback *sc = g_new0 (SignalCallback, 1);
    sc->signal = signal;
    sc->callback = callback;
    sc->data = data;
    plugin->cb = sc;
}

static void
show_all_clicked (GtkWidget *button, gpointer data)
{
    do_window_actions(1);
}


static void
hide_all_clicked (GtkWidget *button, gpointer data)
{
    do_window_actions(0);
}

gui *plugin_gui;
static GtkTooltips *tooltips = NULL;

static gui *
gui_new (void)
{
    plugin_gui = g_new(gui, 1);
    plugin_gui->swapCommands = FALSE;
    plugin_gui->showTooltips = TRUE;
    plugin_gui->lessSpace = FALSE;
    plugin_gui->ebox = gtk_event_box_new();
    gtk_widget_show (plugin_gui->ebox);
 
    plugin_gui->box = gtk_hbox_new(0, 0);

    gtk_widget_show (plugin_gui->box);
        
    plugin_gui->show_all = gtk_button_new ();
    plugin_gui->hide_all = gtk_button_new ();

    gtk_container_add (GTK_CONTAINER(plugin_gui->ebox), plugin_gui->box);
	
    return(plugin_gui);
}

static gboolean
create_plugin_control (Control *ctrl)
{
    gui *plugin;

    plugin = gui_new();

    gtk_container_add (GTK_CONTAINER(ctrl->base), plugin->ebox);

    ctrl->data = (gpointer)plugin;
    ctrl->with_popup = FALSE;

    gtk_widget_set_size_request (ctrl->base, -1, -1);

    return(TRUE);
}

static void
plugin_free (Control *ctrl)
{
    gui *plugin;

    g_return_if_fail (ctrl != NULL);
    g_return_if_fail (ctrl->data != NULL);

    plugin = (gui *)ctrl->data;

    g_free(plugin);
}

static void
plugin_attach_callback (Control *ctrl, const gchar *signal, GCallback cb, gpointer data)
{
    gui *plugin;
    plugin = (gui *)ctrl->data;
    g_signal_connect (plugin->ebox, signal, cb, data);
    g_signal_connect (plugin->show_all, signal, cb, data);
    g_signal_connect (plugin->hide_all, signal, cb, data);
    track_callback (signal, cb, data, plugin);
}

static void
plugin_set_size (Control *ctrl, int size)
{
    // do nothing, cause it's only called once during plugin initialization,
    // instead do resizing during plugin_recreate_gui
}

static void
plugin_recreate_tooltips ()
{
    if (plugin_gui->showTooltips) {
        tooltips = gtk_tooltips_new ();
	if (plugin_gui->swapCommands) {
	    gtk_tooltips_set_tip (tooltips, plugin_gui->hide_all, "Show all windows", NULL);
            gtk_tooltips_set_tip (tooltips, plugin_gui->show_all, "Show desktop", NULL);
        } else {
            gtk_tooltips_set_tip (tooltips, plugin_gui->show_all, "Show all windows", NULL);
            gtk_tooltips_set_tip (tooltips, plugin_gui->hide_all, "Show desktop", NULL);
        }
    }
}

static void
plugin_style_changed (void)
{
    GdkPixbuf *tmp, *pb;

    gtk_widget_destroy (plugin_gui->show_image);
    gtk_widget_destroy (plugin_gui->hide_image);

    if (plugin_gui->orientation == HORIZONTAL) {
        tmp = gtk_widget_render_icon (plugin_gui->ebox, GTK_STOCK_GO_UP, GTK_ICON_SIZE_MENU, NULL);
	pb = gdk_pixbuf_scale_simple (tmp, 12, 12, GDK_INTERP_BILINEAR);
	plugin_gui->show_image = gtk_image_new_from_pixbuf (pb);
	g_object_unref (tmp);
	
        tmp = gtk_widget_render_icon (plugin_gui->ebox, GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_MENU, NULL);
	pb = gdk_pixbuf_scale_simple (tmp, 12, 12, GDK_INTERP_BILINEAR);
	plugin_gui->hide_image = gtk_image_new_from_pixbuf (pb);
	g_object_unref (tmp);
    } else {
        tmp = gtk_widget_render_icon (plugin_gui->ebox, GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_MENU, NULL);
	pb = gdk_pixbuf_scale_simple (tmp, 12, 12, GDK_INTERP_BILINEAR);
	plugin_gui->show_image = gtk_image_new_from_pixbuf (pb);
	g_object_unref (tmp);

        tmp = gtk_widget_render_icon (plugin_gui->ebox, GTK_STOCK_GO_BACK, GTK_ICON_SIZE_MENU, NULL);
	pb = gdk_pixbuf_scale_simple (tmp, 12, 12, GDK_INTERP_BILINEAR);
	plugin_gui->hide_image = gtk_image_new_from_pixbuf (pb);
	g_object_unref (tmp);
    } 
    gtk_container_add (GTK_CONTAINER(plugin_gui->show_all), plugin_gui->show_image);
    gtk_container_add (GTK_CONTAINER(plugin_gui->hide_all), plugin_gui->hide_image);

    gtk_widget_show (plugin_gui->show_image);
    gtk_widget_show (plugin_gui->hide_image);
}
        
static void
plugin_recreate_gui (void)
{
    SignalCallback *sc;
    GdkPixbuf *tmp, *pb;
    
    gtk_widget_destroy (plugin_gui->box);

    plugin_gui->show_all = gtk_button_new ();
    plugin_gui->hide_all = gtk_button_new ();

    if (plugin_gui->orientation == HORIZONTAL) {
        if (plugin_gui->lessSpace) {
	    plugin_gui->box = gtk_vbox_new (0, 0);
	    gtk_widget_set_size_request (plugin_gui->ebox, 15, -1);
	} else {
	    plugin_gui->box = gtk_hbox_new (0, 0);
	    gtk_widget_set_size_request (plugin_gui->ebox, 30, -1);
	}
	tmp = gtk_widget_render_icon (plugin_gui->ebox, GTK_STOCK_GO_UP, GTK_ICON_SIZE_MENU, NULL);
	pb = gdk_pixbuf_scale_simple (tmp, 12, 12, GDK_INTERP_BILINEAR);
	plugin_gui->show_image = gtk_image_new_from_pixbuf (pb);
	g_object_unref (tmp);
	gtk_widget_set_size_request (plugin_gui->show_all, 5, 5);
	
        tmp = gtk_widget_render_icon (plugin_gui->ebox, GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_MENU, NULL);
	pb = gdk_pixbuf_scale_simple (tmp, 12, 12, GDK_INTERP_BILINEAR);
	plugin_gui->hide_image = gtk_image_new_from_pixbuf (pb);
	g_object_unref (tmp);
	gtk_widget_set_size_request (plugin_gui->hide_all, 5, 5);
	
        gtk_container_add (GTK_CONTAINER(plugin_gui->box), plugin_gui->show_all);
        gtk_container_add (GTK_CONTAINER(plugin_gui->box), plugin_gui->hide_all);
    } else {
        if (plugin_gui->lessSpace) {
	    plugin_gui->box = gtk_hbox_new (0, 0);
	    gtk_widget_set_size_request (plugin_gui->ebox, -1, 15);
	} else {
	    plugin_gui->box = gtk_vbox_new (0, 0);
	    gtk_widget_set_size_request (plugin_gui->ebox, -1, 30);
	}
	
        tmp = gtk_widget_render_icon (plugin_gui->ebox, GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_MENU, NULL);
	pb = gdk_pixbuf_scale_simple (tmp, 12, 12, GDK_INTERP_BILINEAR);
	plugin_gui->show_image = gtk_image_new_from_pixbuf (pb);
	g_object_unref (tmp);
	gtk_widget_set_size_request (plugin_gui->show_all, 5, 5);

        tmp = gtk_widget_render_icon (plugin_gui->ebox, GTK_STOCK_GO_BACK, GTK_ICON_SIZE_MENU, NULL);
	pb = gdk_pixbuf_scale_simple (tmp, 12, 12, GDK_INTERP_BILINEAR);
	plugin_gui->hide_image = gtk_image_new_from_pixbuf (pb);
	g_object_unref (tmp);
	gtk_widget_set_size_request (plugin_gui->hide_all, 5, 5);
	
        gtk_container_add (GTK_CONTAINER(plugin_gui->box), plugin_gui->hide_all);
        gtk_container_add (GTK_CONTAINER(plugin_gui->box), plugin_gui->show_all);
    }		
    
    gtk_container_add (GTK_CONTAINER(plugin_gui->show_all), plugin_gui->show_image);
    gtk_button_set_relief (GTK_BUTTON (plugin_gui->show_all), GTK_RELIEF_NONE);
    gtk_container_add (GTK_CONTAINER(plugin_gui->hide_all), plugin_gui->hide_image);
    gtk_button_set_relief (GTK_BUTTON (plugin_gui->hide_all), GTK_RELIEF_NONE);
    gtk_container_add (GTK_CONTAINER(plugin_gui->ebox), plugin_gui->box);

    gtk_widget_show (plugin_gui->box);
    gtk_widget_show (plugin_gui->hide_all);
    gtk_widget_show (plugin_gui->show_all);
    gtk_widget_show (plugin_gui->hide_image);
    gtk_widget_show (plugin_gui->show_image);

    if (plugin_gui->swapCommands) {
        g_signal_connect (plugin_gui->show_all, "clicked", G_CALLBACK(hide_all_clicked), plugin_gui);
        g_signal_connect (plugin_gui->hide_all, "clicked", G_CALLBACK(show_all_clicked), plugin_gui);
    } else {
        g_signal_connect (plugin_gui->show_all, "clicked", G_CALLBACK(show_all_clicked), plugin_gui);
        g_signal_connect (plugin_gui->hide_all, "clicked", G_CALLBACK(hide_all_clicked), plugin_gui);
    }

    g_signal_connect (plugin_gui->ebox, "style_set", G_CALLBACK(plugin_style_changed), NULL);

    plugin_recreate_tooltips ();

    sc = plugin_gui->cb;
    g_signal_connect (plugin_gui->show_all, sc->signal, sc->callback, sc->data);
    g_signal_connect (plugin_gui->hide_all, sc->signal, sc->callback, sc->data);
}

static void
plugin_read_config (Control *ctrl, xmlNodePtr parent)
{   
    xmlChar *swap;
    xmlChar *tool;
    xmlChar *space;

    tool = xmlGetProp (parent, (const xmlChar *) "showTooltips");

    if (tool) {
        if (!strcmp (tool, "1")) {
            plugin_gui->showTooltips = FALSE;
	}
    } else {
            plugin_gui->showTooltips = TRUE;
    }
     
    // to be backward compatible 
    if (xmlHasProp (parent, (const xmlChar *) "swapPixmaps") != NULL) {
        swap = xmlGetProp (parent, (const xmlChar *) "swapPixmaps");
    } else {
        swap = xmlGetProp (parent, (const xmlChar *) "swapCommands");
    }

    if (swap) {
        if (!strcmp (swap, "0")) {
            plugin_gui->swapCommands = TRUE;
        }
    } else {
            plugin_gui->swapCommands = FALSE;
    }

    space = xmlGetProp (parent, (const xmlChar *) "lessSpace");
    
    if (space) {
        if (!strcmp (space, "0")) {
            plugin_gui->lessSpace = TRUE;
        }
    } else {
            plugin_gui->lessSpace = FALSE;
    }

    g_free (swap);
    g_free (tool);
    g_free (space);
    plugin_recreate_gui ();
}

static void
plugin_write_config (Control *ctrl, xmlNodePtr parent)
{   
    char swap[2];
    char tool[2];
    char size[2];
    
    if (plugin_gui->swapCommands) {
        g_snprintf (swap, 2, "%i", 0);
    } else {
        g_snprintf (swap, 2, "%i", 1);
    }
    
    if (plugin_gui->showTooltips) {
        g_snprintf (tool, 2, "%i", 0);
    } else {
        g_snprintf (tool, 2, "%i", 1);
    }
    
    if (plugin_gui->lessSpace) {
        g_snprintf (size, 2, "%i", 0);
    } else {
        g_snprintf (size, 2, "%i", 1);
    }
 
    xmlSetProp (parent, (const xmlChar *) "swapCommands", swap);
    xmlSetProp (parent, (const xmlChar *) "showTooltips", tool);
    xmlSetProp (parent, (const xmlChar *) "lessSpace", size);
}

static void
plugin_set_orientation (Control *ctrl, int orientation)
{
    if (plugin_gui->swapCommands) {
        if (orientation == HORIZONTAL) {
	    plugin_gui->orientation = HORIZONTAL;
	    plugin_gui->swapCommands = TRUE;
	} else if (orientation == VERTICAL) {
	    plugin_gui->orientation = VERTICAL;
	    plugin_gui->swapCommands = TRUE;
	}
    } else {
        if (orientation == HORIZONTAL) {
	    plugin_gui->orientation = HORIZONTAL;
	    plugin_gui->swapCommands = FALSE;
	} else if (orientation == VERTICAL) {
	    plugin_gui->orientation = VERTICAL;
	    plugin_gui->swapCommands = FALSE;
	}
    }
    plugin_recreate_gui ();
}

static void
plugin_cb1_changed (GtkToggleButton *cb)
{
    gboolean swapCommands;

    swapCommands = gtk_toggle_button_get_active (cb);

    if (swapCommands) {
        plugin_gui->swapCommands = TRUE;
    } else {
	plugin_gui->swapCommands = FALSE;
    }
    plugin_recreate_gui ();
}

static void
plugin_cb2_changed (GtkToggleButton *cb)
{
    gboolean showTooltips;

    showTooltips = gtk_toggle_button_get_active (cb);
    plugin_gui->showTooltips = showTooltips;

    if (showTooltips) {
        gtk_tooltips_enable (tooltips);
	plugin_recreate_tooltips ();
    } else {
        gtk_tooltips_disable (tooltips);
    }
}

static void
plugin_cb3_changed (GtkToggleButton *cb)
{
    gboolean lessSpace;

    lessSpace = gtk_toggle_button_get_active (cb);

    if (lessSpace) {
	plugin_gui->lessSpace = TRUE;
    } else {
        plugin_gui->lessSpace = FALSE;
    }
    plugin_recreate_gui ();
}


static void
plugin_create_options (Control *ctrl, GtkContainer *con, GtkWidget *done)
{
    GtkWidget *vbox, *label, *cb1, *cb2, *cb3;

    vbox = gtk_vbox_new (1, 1);
    gtk_widget_show (vbox);

    cb1 = gtk_check_button_new_with_label ("swap commands");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb1), plugin_gui->swapCommands);
    g_signal_connect (cb1, "toggled", G_CALLBACK (plugin_cb1_changed), NULL);
    gtk_widget_show (cb1);
    
    cb2 = gtk_check_button_new_with_label ("show tooltips");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb2), plugin_gui->showTooltips);
    g_signal_connect (cb2, "toggled", G_CALLBACK (plugin_cb2_changed), NULL);
    gtk_widget_show (cb2);
    
    cb3 = gtk_check_button_new_with_label ("reduce size");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb3), plugin_gui->lessSpace);
    g_signal_connect (cb3, "toggled", G_CALLBACK (plugin_cb3_changed), NULL);
    gtk_widget_show (cb3);

    gtk_container_add (con, vbox);
    gtk_container_add (GTK_CONTAINER(vbox), cb1);
    gtk_container_add (GTK_CONTAINER(vbox), cb2);
    gtk_container_add (GTK_CONTAINER(vbox), cb3);
}

// }}}

// initialization {{{
G_MODULE_EXPORT void
xfce_control_class_init(ControlClass *cc)
{
    /* these are required */
    cc->name		= "showdesktop";
    cc->caption		= _("Show Desktop");

    cc->create_control	= (CreateControlFunc)create_plugin_control;

    cc->free		= plugin_free;
    cc->attach_callback	= plugin_attach_callback;

    /* options; don't define if you don't have any ;) */
    cc->read_config	= plugin_read_config;
    cc->write_config	= plugin_write_config;

    /* Don't use this function at all if you want xfce to
     * do the sizing.
     * Just define the set_size function to NULL, or rather, don't 
     * set it to something else.
    */
    cc->set_size		= plugin_set_size;
    cc->create_options          = plugin_create_options;
    cc->set_orientation		= plugin_set_orientation;
}

/* required! defined in panel/plugins.h */
XFCE_PLUGIN_CHECK_INIT

// }}}

// vim600: set foldmethod=marker: foldmarker={{{,}}}
