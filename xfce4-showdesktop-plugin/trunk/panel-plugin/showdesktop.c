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

// struct {{{

typedef struct
{
    GtkWidget	*show_all;
    GtkWidget	*hide_all;
    GtkWidget	*show_hide;
    GtkWidget   *base;
    GtkWidget	*ebox;
    gint        orientation;
    gint        IconSize;
    gboolean    swapCommands;
    gboolean    showTooltips;
    gboolean    lessSpace;
    gboolean	oneButton;
    gboolean	hide;
    GList       *windows;
    void (*function) (GtkWidget *widget, GdkEventButton *ev, Control *ctrl);
    Control *ctrl;
} gui;

static GtkTooltips *tooltips = NULL;
static GtkTooltips *optiontooltips = NULL;

// }}}

// all functions {{{

         /* 
	    do_window_actions: types:
	    0... hide all windows
	    1... show all windows
	    2... hide the last window group
	    3... show the last window group
	 */

static void
do_window_actions (int type, gpointer data)
{
    gui *p = data;
    NetkScreen *screen;
    NetkWorkspace *workspace;
    NetkWindow *window;
    NetkWindow *active_window = NULL;
    
    GList *w = NULL;
    
    screen = netk_screen_get_default();
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
                if (type == 0 || type == 2) {
		    if (!netk_window_is_minimized (window)) {
	                netk_window_minimize (window);
		        p->windows = g_list_append (p->windows, window);
		    }
	        } else if (type == 1 || type == 3) {
		    if (netk_window_is_minimized (window)) {
	                netk_window_unminimize (window);
			active_window=window;
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
}

static void
show_all_clicked (GtkWidget *button, GdkEventButton *ev, gpointer data)
{
    gui *plugin=data;
    if (ev->button == 1) {
        plugin->swapCommands ? do_window_actions(0, data) : do_window_actions(1, data);
    } else if (ev->button == 2) {
        plugin->swapCommands ? do_window_actions(2, data) : do_window_actions(3, data);
    } else if (ev->button == 3) {
	plugin->function(button, ev, plugin->ctrl);
    }
}

static void
hide_all_clicked (GtkWidget *button, GdkEventButton *ev, gpointer data)
{
    gui *plugin=data;
    if (ev->button == 1) {
        plugin->swapCommands ? do_window_actions(1, data) : do_window_actions (0, data);
    } else if (ev->button == 2) {
        plugin->swapCommands ? do_window_actions(3, data) : do_window_actions(2, data);
    } else if (ev->button == 3) {
	plugin->function(button, ev, plugin->ctrl);
    }
}

static void
show_hide_clicked (GtkWidget *button, GdkEventButton *ev, gpointer data)
{
    gui *plugin = data;
    GdkPixbuf *tmp, *pb0, *pb1;
    if (plugin->orientation == HORIZONTAL) {
        tmp = gtk_widget_render_icon (plugin->ebox, GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_MENU, NULL);
	pb0 = gdk_pixbuf_scale_simple (tmp, plugin->IconSize, plugin->IconSize, GDK_INTERP_BILINEAR);
	g_object_unref (tmp);
	
    	tmp = gtk_widget_render_icon (plugin->ebox, GTK_STOCK_GO_UP, GTK_ICON_SIZE_MENU, NULL);
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

    if (ev->button == 1) {
	if (plugin->hide) {
	    xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON(plugin->show_hide), pb1);
            plugin->swapCommands ? do_window_actions(1, data) : do_window_actions (0, data);
	} else {
	    xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON(plugin->show_hide), pb0);
            plugin->swapCommands ? do_window_actions(0, data) : do_window_actions(1, data);
	}
	plugin->hide=!plugin->hide;
    } else if (ev->button == 2) {
	if (plugin->hide) {
	    xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON(plugin->show_hide), pb1);
            plugin->swapCommands ? do_window_actions(3, data) : do_window_actions(2, data);
	} else {
	    xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON(plugin->show_hide), pb0);
            plugin->swapCommands ? do_window_actions(2, data) : do_window_actions(3, data);
	}
	plugin->hide=!plugin->hide;
    } else if (ev->button == 3) {
	plugin->function(button, ev, plugin->ctrl);
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
    plugin->oneButton = FALSE;
    plugin->hide = TRUE;
    plugin->IconSize = ICONSIZETINY;
    plugin->ebox = gtk_event_box_new();
    plugin->windows = NULL;
    gtk_widget_show (plugin->ebox);
    
    plugin->base = gtk_hbox_new(FALSE, 0);
    gtk_widget_show (plugin->base);
    
    gtk_container_add (GTK_CONTAINER(plugin->ebox), plugin->base);
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
    gui *plugin;
    
    g_return_if_fail (ctrl != NULL);
    g_return_if_fail (ctrl->data != NULL);

    plugin = ctrl->data;
    g_free(plugin);
}

static void
plugin_attach_callback (Control *ctrl, const gchar *signal, GCallback cb, gpointer data)
{
    gui *plugin = ctrl->data;
    g_signal_connect (plugin->ebox, signal, cb, data);
    plugin->function=(void *)cb;
    plugin->ctrl=ctrl;
}

static void
plugin_recreate_tooltips (gui *plugin)
{
    if (plugin->showTooltips) {
        tooltips = gtk_tooltips_new ();
	if (plugin->oneButton) {
	    gtk_tooltips_set_tip (tooltips, plugin->show_hide,
	      (_("Press this button to show/hide windows")), NULL);
	} else {
	    if (plugin->swapCommands) {
	        gtk_tooltips_set_tip (tooltips, plugin->hide_all,
	          N_("Button 1: show all windows\nButton 2: show previous window group"), NULL);
                gtk_tooltips_set_tip (tooltips, plugin->show_all,
	          N_("Button 1: hide all windows\nButton 2: hide previous window group"), NULL);
            } else {
                gtk_tooltips_set_tip (tooltips, plugin->show_all,
	          N_("Button 1: show all windows\nButton 2: show previous window group"), NULL);
                gtk_tooltips_set_tip (tooltips, plugin->hide_all,
	          N_("Button 1: hide all windows\nButton 2: hide previous window group"), NULL);
            }
	}
    }
}

static void
plugin_style_changed (GtkWidget *widget, GtkStyle *style, gui *plugin)
{
    GdkPixbuf *tmp, *pb0, *pb1, *pb2, *pb3, *pb4;

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

    if (plugin->oneButton) {
	if (plugin->hide) {
		xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON(plugin->show_hide), pb1);
	} else {
		xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON(plugin->show_hide), pb0);
	}
    } else {
	xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON(plugin->show_all), pb0);
	xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON(plugin->hide_all), pb1);
    }

} 
   
static void
plugin_recreate_gui (gui *plugin)
{
    GdkPixbuf *tmp, *pb0, *pb1, *pb2, *pb3, *pb4;

    gtk_widget_destroy (plugin->base);

    plugin->show_all = xfce_iconbutton_new ();
    plugin->hide_all = xfce_iconbutton_new ();
    plugin->show_hide = xfce_iconbutton_new ();
    
    if (plugin->orientation == HORIZONTAL) {
    	tmp = gtk_widget_render_icon (plugin->ebox, GTK_STOCK_GO_UP, GTK_ICON_SIZE_MENU, NULL);
	pb0 = gdk_pixbuf_scale_simple (tmp, plugin->IconSize, plugin->IconSize, GDK_INTERP_BILINEAR);
	g_object_unref (tmp);
	
        tmp = gtk_widget_render_icon (plugin->ebox, GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_MENU, NULL);
	pb1 = gdk_pixbuf_scale_simple (tmp, plugin->IconSize, plugin->IconSize, GDK_INTERP_BILINEAR);
	g_object_unref (tmp);

        if (plugin->lessSpace) {
	    plugin->base = gtk_vbox_new (FALSE, 0);
            gtk_widget_set_size_request (plugin->show_all, plugin->IconSize * 2, plugin->IconSize);
            gtk_widget_set_size_request (plugin->hide_all, plugin->IconSize * 2, plugin->IconSize);
	    gtk_widget_set_size_request (plugin->show_hide, plugin->IconSize * 2, plugin->IconSize);
	} else {
	    plugin->base = gtk_hbox_new (FALSE, 0);
            gtk_widget_set_size_request (plugin->show_all, plugin->IconSize * 2, -1);
	    gtk_widget_set_size_request (plugin->hide_all, plugin->IconSize * 2, -1);
	    gtk_widget_set_size_request (plugin->show_hide, plugin->IconSize * 2, -1);
	}
    } else {
        tmp = gtk_widget_render_icon (plugin->ebox, GTK_STOCK_GO_BACK, GTK_ICON_SIZE_MENU, NULL);
	pb0 = gdk_pixbuf_scale_simple (tmp, plugin->IconSize, plugin->IconSize, GDK_INTERP_BILINEAR);
	g_object_unref (tmp);

        tmp = gtk_widget_render_icon (plugin->ebox, GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_MENU, NULL);
	pb1 = gdk_pixbuf_scale_simple (tmp, plugin->IconSize, plugin->IconSize, GDK_INTERP_BILINEAR);
	g_object_unref (tmp);
	
        if (plugin->lessSpace) {
	    plugin->base = gtk_hbox_new (FALSE, 0);
	    gtk_widget_set_size_request (plugin->show_all, plugin->IconSize, plugin->IconSize * 2);
	    gtk_widget_set_size_request (plugin->hide_all, plugin->IconSize, plugin->IconSize * 2);
	    gtk_widget_set_size_request (plugin->show_hide, plugin->IconSize, plugin->IconSize * 2);
	} else {
	    plugin->base = gtk_vbox_new (FALSE, 0);
	    gtk_widget_set_size_request (plugin->show_all, -1, plugin->IconSize * 2);
	    gtk_widget_set_size_request (plugin->hide_all, -1, plugin->IconSize * 2);
	    gtk_widget_set_size_request (plugin->show_hide, -1, plugin->IconSize * 2);
	}
    }	
    
    if (plugin->oneButton) {
        gtk_container_add (GTK_CONTAINER(plugin->base), plugin->show_hide);
        if (plugin->hide) {
            xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON(plugin->show_hide), pb1);
        } else {
            xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON(plugin->show_hide), pb0);
        }
    	gtk_button_set_relief (GTK_BUTTON (plugin->show_hide), GTK_RELIEF_NONE);
        g_signal_connect (plugin->show_hide, "button_press_event", G_CALLBACK(show_hide_clicked), plugin);
    } else {
        xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON(plugin->show_all), pb0);
        xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON(plugin->hide_all), pb1);
        gtk_button_set_relief (GTK_BUTTON (plugin->hide_all), GTK_RELIEF_NONE);
        gtk_button_set_relief (GTK_BUTTON (plugin->show_all), GTK_RELIEF_NONE);
        gtk_container_add (GTK_CONTAINER(plugin->base), plugin->show_all);
        gtk_container_add (GTK_CONTAINER(plugin->base), plugin->hide_all);
        g_signal_connect (plugin->hide_all, "button_press_event", G_CALLBACK(hide_all_clicked), plugin);
        g_signal_connect (plugin->show_all, "button_press_event", G_CALLBACK(show_all_clicked), plugin);
    }

    gtk_container_add (GTK_CONTAINER(plugin->ebox), plugin->base);
    gtk_widget_show_all (plugin->base);
    g_signal_connect (plugin->ebox, "style_set", G_CALLBACK(plugin_style_changed), plugin);

    plugin_recreate_tooltips (plugin);
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
    xmlChar *oneb;
    xmlChar *tool;
    xmlChar *space;
    gui *plugin = ctrl->data;
    
    oneb = xmlGetProp (parent, (const xmlChar *) "oneButton");
    
    if (oneb) {
        if (!strcmp (oneb, "0")) {
            plugin->oneButton = TRUE;
        }
    } else {
            plugin->oneButton = FALSE;
    }
 
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
    g_free (oneb);
    plugin_recreate_gui (plugin);
}

static void
plugin_write_config (Control *ctrl, xmlNodePtr parent)
{   
    gui *plugin = ctrl->data;
    char swap[2];
    char tool[2];
    char size[2];
    char oneb[2];
    
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

    if (plugin->oneButton) {
        g_snprintf (oneb, 2, "%i", 0);
    } else {
        g_snprintf (oneb, 2, "%i", 1);
    }

    xmlSetProp (parent, (const xmlChar *) "swapCommands", swap);
    xmlSetProp (parent, (const xmlChar *) "showTooltips", tool);
    xmlSetProp (parent, (const xmlChar *) "lessSpace", size);
    xmlSetProp (parent, (const xmlChar *) "oneButton", oneb);
}

static void
plugin_set_orientation (Control *ctrl, int orientation)
{
    gui *plugin = ctrl->data;
    plugin->orientation = orientation;
    plugin_recreate_gui (plugin);
}

static void
plugin_cb1_changed (GtkToggleButton *cb, gui *plugin)
{
    gboolean swapCommands;

    swapCommands = gtk_toggle_button_get_active (cb);
    plugin->swapCommands = swapCommands;

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
    plugin->lessSpace = lessSpace;
    
    plugin_recreate_gui (plugin);
}

static void
plugin_cb4_changed (GtkToggleButton *cb, gui *plugin)
{
    gboolean oneButton;

    oneButton = gtk_toggle_button_get_active (cb);
    plugin->oneButton = oneButton;

    plugin_recreate_gui (plugin);
}

static void
plugin_create_options (Control *ctrl, GtkContainer *con, GtkWidget *done)
{
    gui *plugin = ctrl->data;
    GtkWidget *vbox, *optionbox, *cb1, *cb2, *cb3, *cb4, *frame;
    
    optiontooltips = gtk_tooltips_new ();

    gchar *tips[] = {
        N_("Enable this options, if your panel is located on top or on the right."),
        N_("Enable this option, if you want tooltips on all command buttons."),
        N_("If this options is enabled, the plugin consumes less space on the panel."),
        N_("one button only"),
    };

    vbox = gtk_vbox_new (False, 5);
    
    cb1 = gtk_check_button_new_with_label N_("swap commands");
    gtk_tooltips_set_tip (optiontooltips, cb1, tips[0], NULL);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb1), plugin->swapCommands);
    g_signal_connect (cb1, "toggled", G_CALLBACK (plugin_cb1_changed), plugin);
    
    cb2 = gtk_check_button_new_with_label N_("show tooltips");
    gtk_tooltips_set_tip (optiontooltips, cb2, tips[1], NULL);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb2), plugin->showTooltips);
    g_signal_connect (cb2, "toggled", G_CALLBACK (plugin_cb2_changed), plugin);
    
    cb3 = gtk_check_button_new_with_label N_("reduce size");
    gtk_tooltips_set_tip (optiontooltips, cb3, tips[2], NULL);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb3), plugin->lessSpace);
    g_signal_connect (cb3, "toggled", G_CALLBACK (plugin_cb3_changed), plugin);

    cb4 = gtk_check_button_new_with_label ("one toggle button");
    gtk_tooltips_set_tip (optiontooltips, cb4, tips[3], NULL);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb4), plugin->oneButton);
    g_signal_connect (cb4, "toggled", G_CALLBACK (plugin_cb4_changed), plugin);

    optionbox = gtk_vbox_new (False, 1);
    frame = gtk_frame_new N_("Options");
 
    gtk_container_add (con, vbox);
    gtk_container_add (GTK_CONTAINER(optionbox), cb1);
    gtk_container_add (GTK_CONTAINER(optionbox), cb2);
    gtk_container_add (GTK_CONTAINER(optionbox), cb3);
    gtk_container_add (GTK_CONTAINER(optionbox), cb4);
    gtk_container_add (GTK_CONTAINER(frame), optionbox);
    gtk_container_add (GTK_CONTAINER(vbox), frame);
    gtk_widget_show_all (vbox);
}
// }}}

// initialization {{{
G_MODULE_EXPORT void
xfce_control_class_init(ControlClass *cc)
{
    xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    /* these are required */
    cc->name		= "showdesktop";
    cc->caption		= N_("Show Desktop");

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
