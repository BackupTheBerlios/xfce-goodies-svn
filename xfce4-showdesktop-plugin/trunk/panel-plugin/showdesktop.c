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

// some includes and defines {{{

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

#define TINY 0
#define SMALL 1
#define MEDIUM 2
#define LARGE 3

#define ICONSIZETINY 8
#define ICONSIZESMALL 10
#define ICONSIZEMEDIUM 15
#define ICONSIZELARGE 20


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
    GtkWidget   *box;
    GtkWidget	*ebox;
    gint        orientation;
    gint        IconSize;
    gboolean    swapCommands;
    gboolean    showTooltips;
    gboolean    lessSpace;
    gboolean    unhide;
    GList       *windows;
    SignalCallback *cb;
} gui;

static GtkTooltips *tooltips = NULL;

// }}}

// all functions {{{

         /* 
	    do_window_actions: minimize or unminimize all unsticky windows on
	    the current workspace
	 */

int
do_window_actions (int type, gpointer data)
{
    gui *p = data;
    NetkScreen *screen;
    NetkWorkspace *workspace;
    NetkWindow *window;
    NetkWindow *active_window = NULL;
    GList *w;
    
    screen = netk_screen_get (0);
    workspace = netk_screen_get_active_workspace (screen);
    if (type == 2 || type == 3) {
        w = p->windows;
    } else {
        w = netk_screen_get_windows_stacked (screen);
    }

    p->windows = NULL;

    while (w != NULL) {
        window = w->data;
	if (!NETK_IS_WINDOW(window)) {
 	    w = w->next;
	    continue;
	}
        if (!netk_window_is_sticky (window)) {
            if (workspace == netk_window_get_workspace (window)) {
		active_window = window;
                if (type == 0 || type == 2) {
		    if (!netk_window_is_minimized (window)) {
	                netk_window_minimize (window);
		        p->windows = g_list_append (p->windows, window);
		    }
	        } else {
		    if (netk_window_is_minimized (window)) {
	                netk_window_unminimize (window);
		        p->windows = g_list_append (p->windows, window);
	            }
	        }
	    }
	 }
 	 w = w->next;   
    }
    if (active_window != NULL && (type == 1 || type == 3)) {
        netk_window_activate (active_window);
    }
    if (type == 0) {
        p->unhide = FALSE;
    } else if (type == 1){
        p->unhide = TRUE;
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
show_all_clicked (GtkWidget *button, GdkEventButton *ev, gpointer data)
{
    if (ev->button == 1) {
        do_window_actions(1, data);
    } else if (ev->button == 2) {
        do_window_actions(3, data);
    } else if (ev->button == 3) {
	g_signal_emit_by_name (button, "clicked");
    }
}

static void
hide_all_clicked (GtkWidget *button, GdkEventButton *ev, gpointer data)
{
    if (ev->button == 1) {
        do_window_actions(0, data);
    } else if (ev->button == 2) {
        do_window_actions(2, data);
    } else if (ev->button == 3) {
	g_signal_emit_by_name (button, "clicked");
    }
}

static gui *
gui_new ()
{
    gui *plugin;
    plugin = g_new(gui, 1);
    plugin->swapCommands = FALSE;
    plugin->showTooltips = TRUE;
    plugin->lessSpace = FALSE;
    plugin->IconSize = 8;
    plugin->ebox = gtk_event_box_new();
    plugin->windows = NULL;
    gtk_widget_show (plugin->ebox);
 
    plugin->box = gtk_hbox_new(FALSE, 0);

    gtk_widget_show (plugin->box);
        
    plugin->show_all = xfce_iconbutton_new ();
    plugin->hide_all = xfce_iconbutton_new ();

    gtk_container_add (GTK_CONTAINER(plugin->ebox), plugin->box);
	
    return(plugin);
}

static gboolean
create_plugin_control (Control *ctrl)
{
    gui *plugin = gui_new();

    gtk_container_add (GTK_CONTAINER(ctrl->base), plugin->ebox);

    ctrl->data = (gpointer)plugin;
    ctrl->with_popup = FALSE;

    gtk_widget_set_size_request (ctrl->base, -1, -1);

    return(TRUE);
}

static void
plugin_free (Control *ctrl)
{
    g_return_if_fail (ctrl != NULL);
    g_return_if_fail (ctrl->data != NULL);

    gui *plugin = ctrl->data;
    g_free(plugin);
}

static void
plugin_attach_callback (Control *ctrl, const gchar *signal, GCallback cb, gpointer data)
{
    gui *plugin = ctrl->data;
    g_signal_connect (plugin->ebox, signal, cb, data);
    g_signal_connect (plugin->show_all, signal, cb, data);
    g_signal_connect (plugin->hide_all, signal, cb, data);
    track_callback (signal, cb, data, plugin);
}

static void
plugin_recreate_tooltips (gui *plugin)
{
    if (plugin->showTooltips) {
        tooltips = gtk_tooltips_new ();
	if (plugin->swapCommands) {
	    gtk_tooltips_set_tip (tooltips, plugin->hide_all,
	      (_("Button 1: show all windows\nButton 2: show previous window group")), NULL);
            gtk_tooltips_set_tip (tooltips, plugin->show_all,
	      (_("Button 1: hide all windows\nButton 2: hide previous window group")), NULL);
        } else {
            gtk_tooltips_set_tip (tooltips, plugin->show_all,
	      (_("Button 1: show all windows\nButton 2: show previous window group")), NULL);
            gtk_tooltips_set_tip (tooltips, plugin->hide_all,
	      (_("Button 1: hide all windows\nButton 2: hide previous window group")), NULL);
        }
    }
}

static void
plugin_style_changed (GtkWidget *widget, GtkStyle *style, gui *plugin)
{
    GdkPixbuf *tmp, *pb0, *pb1;

    if (plugin->orientation == HORIZONTAL) {
        tmp = gtk_widget_render_icon (plugin->ebox, GTK_STOCK_GO_UP, GTK_ICON_SIZE_MENU, NULL);
	pb0 = gdk_pixbuf_scale_simple (tmp, plugin->IconSize, plugin->IconSize, GDK_INTERP_BILINEAR);
	g_object_unref (tmp);
	
        tmp = gtk_widget_render_icon (plugin->ebox, GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_MENU, NULL);
	pb1 = gdk_pixbuf_scale_simple (tmp, plugin->IconSize, plugin->IconSize, GDK_INTERP_BILINEAR);
	g_object_unref (tmp);
    } else {
        tmp = gtk_widget_render_icon (plugin->ebox, GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_MENU, NULL);
	pb0 = gdk_pixbuf_scale_simple (tmp, plugin->IconSize, plugin->IconSize, GDK_INTERP_BILINEAR);
	g_object_unref (tmp);

        tmp = gtk_widget_render_icon (plugin->ebox, GTK_STOCK_GO_BACK, GTK_ICON_SIZE_MENU, NULL);
	pb1 = gdk_pixbuf_scale_simple (tmp, plugin->IconSize, plugin->IconSize, GDK_INTERP_BILINEAR);
	g_object_unref (tmp);
    } 
    xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON(plugin->show_all), pb0);
    xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON(plugin->hide_all), pb1);
} 
   
static void
plugin_recreate_gui (gui *plugin)
{
    SignalCallback *sc;
    GdkPixbuf *tmp, *pb0, *pb1;
    
    gtk_widget_destroy (plugin->box);

    plugin->show_all = xfce_iconbutton_new ();
    plugin->hide_all = xfce_iconbutton_new ();

    if (plugin->orientation == HORIZONTAL) {
    	tmp = gtk_widget_render_icon (plugin->ebox, GTK_STOCK_GO_UP, GTK_ICON_SIZE_MENU, NULL);
	pb0 = gdk_pixbuf_scale_simple (tmp, plugin->IconSize, plugin->IconSize, GDK_INTERP_BILINEAR);
	g_object_unref (tmp);
	
        tmp = gtk_widget_render_icon (plugin->ebox, GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_MENU, NULL);
	pb1 = gdk_pixbuf_scale_simple (tmp, plugin->IconSize, plugin->IconSize, GDK_INTERP_BILINEAR);
	g_object_unref (tmp);

        if (plugin->lessSpace) {
	    plugin->box = gtk_vbox_new (FALSE, 0);
	    gtk_widget_set_size_request (plugin->show_all, plugin->IconSize * 2, plugin->IconSize);
	    gtk_widget_set_size_request (plugin->hide_all, plugin->IconSize * 2, plugin->IconSize);
	} else {
	   plugin->box = gtk_hbox_new (FALSE, 0);
	   gtk_widget_set_size_request (plugin->show_all, plugin->IconSize * 2, -1);
	   gtk_widget_set_size_request (plugin->hide_all, plugin->IconSize * 2, -1);
	}
         
        gtk_container_add (GTK_CONTAINER(plugin->box), plugin->show_all);
        gtk_container_add (GTK_CONTAINER(plugin->box), plugin->hide_all);

    } else {
        tmp = gtk_widget_render_icon (plugin->ebox, GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_MENU, NULL);
	pb0 = gdk_pixbuf_scale_simple (tmp, plugin->IconSize, plugin->IconSize, GDK_INTERP_BILINEAR);
	g_object_unref (tmp);

        tmp = gtk_widget_render_icon (plugin->ebox, GTK_STOCK_GO_BACK, GTK_ICON_SIZE_MENU, NULL);
	pb1 = gdk_pixbuf_scale_simple (tmp, plugin->IconSize, plugin->IconSize, GDK_INTERP_BILINEAR);
	g_object_unref (tmp);
	
        if (plugin->lessSpace) {
	    plugin->box = gtk_hbox_new (FALSE, 0);
	    gtk_widget_set_size_request (plugin->show_all, plugin->IconSize, plugin->IconSize * 2);
	    gtk_widget_set_size_request (plugin->hide_all, plugin->IconSize, plugin->IconSize * 2);
	} else {
	    plugin->box = gtk_vbox_new (FALSE, 0);
	    gtk_widget_set_size_request (plugin->show_all, -1, plugin->IconSize * 2);
	    gtk_widget_set_size_request (plugin->hide_all, -1, plugin->IconSize * 2);
	}

        gtk_container_add (GTK_CONTAINER(plugin->box), plugin->hide_all);
        gtk_container_add (GTK_CONTAINER(plugin->box), plugin->show_all);
    }		
    
    xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON(plugin->show_all), pb0);
    xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON(plugin->hide_all), pb1);
    
    gtk_button_set_relief (GTK_BUTTON (plugin->show_all), GTK_RELIEF_NONE);
    gtk_button_set_relief (GTK_BUTTON (plugin->hide_all), GTK_RELIEF_NONE);
    gtk_container_add (GTK_CONTAINER(plugin->ebox), plugin->box);
    
    gtk_widget_show_all (plugin->box);

    if (plugin->swapCommands) {
        g_signal_connect (plugin->show_all, "button_press_event", G_CALLBACK(hide_all_clicked), plugin);
        g_signal_connect (plugin->hide_all, "button_press_event", G_CALLBACK(show_all_clicked), plugin);
    } else {
        g_signal_connect (plugin->show_all, "button_press_event", G_CALLBACK(show_all_clicked), plugin);
        g_signal_connect (plugin->hide_all, "button_press_event", G_CALLBACK(hide_all_clicked), plugin);
    }

    g_signal_connect (plugin->ebox, "style_set", G_CALLBACK(plugin_style_changed), plugin);

    plugin_recreate_tooltips (plugin);

    sc = plugin->cb;
    g_signal_connect (plugin->show_all, sc->signal, sc->callback, sc->data);
    g_signal_connect (plugin->hide_all, sc->signal, sc->callback, sc->data);
}

static void
plugin_set_size (Control *ctrl, int size)
{
    gui *plugin = ctrl->data;
    if (size == TINY) {
        plugin->IconSize = ICONSIZETINY;
    } else if (size == SMALL) {
        plugin->IconSize = ICONSIZESMALL;
    } else if (size == MEDIUM) {
        plugin->IconSize = ICONSIZEMEDIUM;
    } else {
        plugin->IconSize = ICONSIZELARGE;
    }	
    plugin_recreate_gui (plugin);
}

static void
plugin_read_config (Control *ctrl, xmlNodePtr parent)
{   
    xmlChar *swap;
    xmlChar *tool;
    xmlChar *space;
    gui *plugin = ctrl->data;

    tool = xmlGetProp (parent, (const xmlChar *) "showTooltips");

    if (tool) {
        if (!strcmp (tool, "1")) {
            plugin->showTooltips = FALSE;
	}
    } else {
            plugin->showTooltips = TRUE;
    }
     
    // to be backward compatible 
    if (xmlHasProp (parent, (const xmlChar *) "swapPixmaps") != NULL) {
        swap = xmlGetProp (parent, (const xmlChar *) "swapPixmaps");
    } else {
        swap = xmlGetProp (parent, (const xmlChar *) "swapCommands");
    }

    if (swap) {
        if (!strcmp (swap, "0")) {
            plugin->swapCommands = TRUE;
        }
    } else {
            plugin->swapCommands = FALSE;
    }

    space = xmlGetProp (parent, (const xmlChar *) "lessSpace");
    
    if (space) {
        if (!strcmp (space, "0")) {
            plugin->lessSpace = TRUE;
        }
    } else {
            plugin->lessSpace = FALSE;
    }

    g_free (swap);
    g_free (tool);
    g_free (space);
    plugin_recreate_gui (plugin);
}

static void
plugin_write_config (Control *ctrl, xmlNodePtr parent)
{   
    gui *plugin = ctrl->data;
    char swap[2];
    char tool[2];
    char size[2];
    
    if (plugin->swapCommands) {
        g_snprintf (swap, 2, "%i", 0);
    } else {
        g_snprintf (swap, 2, "%i", 1);
    }
    
    if (plugin->showTooltips) {
        g_snprintf (tool, 2, "%i", 0);
    } else {
        g_snprintf (tool, 2, "%i", 1);
    }
    
    if (plugin->lessSpace) {
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
    gui *plugin = ctrl->data;
    if (plugin->swapCommands) {
        if (orientation == HORIZONTAL) {
	    plugin->orientation = HORIZONTAL;
	    plugin->swapCommands = TRUE;
	} else if (orientation == VERTICAL) {
	    plugin->orientation = VERTICAL;
	    plugin->swapCommands = TRUE;
	}
    } else {
        if (orientation == HORIZONTAL) {
	    plugin->orientation = HORIZONTAL;
	    plugin->swapCommands = FALSE;
	} else if (orientation == VERTICAL) {
	    plugin->orientation = VERTICAL;
	    plugin->swapCommands = FALSE;
	}
    }
    plugin_recreate_gui (plugin);
}

static void
plugin_cb1_changed (GtkToggleButton *cb, gui *plugin)
{
    gboolean swapCommands;

    swapCommands = gtk_toggle_button_get_active (cb);

    if (swapCommands) {
        plugin->swapCommands = TRUE;
    } else {
	plugin->swapCommands = FALSE;
    }
    plugin_recreate_gui (plugin);
}

static void
plugin_cb2_changed (GtkToggleButton *cb, gui *plugin)
{
    gboolean showTooltips;

    showTooltips = gtk_toggle_button_get_active (cb);
    plugin->showTooltips = showTooltips;

    if (showTooltips) {
        gtk_tooltips_enable (tooltips);
	plugin_recreate_tooltips (plugin);
    } else {
        gtk_tooltips_disable (tooltips);
    }
}

static void
plugin_cb3_changed (GtkToggleButton *cb, gui *plugin)
{
    gboolean lessSpace;

    lessSpace = gtk_toggle_button_get_active (cb);

    if (lessSpace) {
	plugin->lessSpace = TRUE;
    } else {
        plugin->lessSpace = FALSE;
    }
    plugin_recreate_gui (plugin);
}


static void
plugin_create_options (Control *ctrl, GtkContainer *con, GtkWidget *done)
{
    gui *plugin = ctrl->data;
    GtkWidget *vbox, *label, *cb1, *cb2, *cb3;

    vbox = gtk_vbox_new (1, 1);
    gtk_widget_show (vbox);

    cb1 = gtk_check_button_new_with_label _("swap commands");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb1), plugin->swapCommands);
    g_signal_connect (cb1, "toggled", G_CALLBACK (plugin_cb1_changed), plugin);
    gtk_widget_show (cb1);
    
    cb2 = gtk_check_button_new_with_label _("show tooltips");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb2), plugin->showTooltips);
    g_signal_connect (cb2, "toggled", G_CALLBACK (plugin_cb2_changed), plugin);
    gtk_widget_show (cb2);
    
    cb3 = gtk_check_button_new_with_label _("reduce size");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb3), plugin->lessSpace);
    g_signal_connect (cb3, "toggled", G_CALLBACK (plugin_cb3_changed), plugin);
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
    xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
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
