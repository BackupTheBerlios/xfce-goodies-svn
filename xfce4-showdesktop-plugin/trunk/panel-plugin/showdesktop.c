// Copyright {{{ 

/*
 * Copyright (c) 2003 Andre Lerche <a.lerche@gmx.net>
 * Copyright (c) 2003 Benedikt Meurer <benedikt.meurer@unix-ag.uni-siegen.de>
 * Copyright (c) 2004 Ariel Flashner <flashner@012.net.il>
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
    gboolean    lessSpace;
    gboolean	oneButton;
    gboolean	hide;
    gboolean    alternate;
    void (*function) (GtkWidget *widget, GdkEventButton *ev, Control *ctrl);
    Control *ctrl;
} gui;

static GtkTooltips *optiontooltips = NULL;

// }}}

// all functions {{{

gboolean 
wm_has_support(Atom supportedAtom, Display *dsp)
{
    gboolean isSupported = FALSE;
    Atom type;
    int format;
    unsigned long n_atoms;
    unsigned long bytes_after;
    Atom *atoms;
    Atom net_supported;
    Display *dpy;

    net_supported = XInternAtom (dsp, "_NET_SUPPORTED", FALSE);

    if ((XGetWindowProperty (dsp, DefaultRootWindow(dsp), net_supported, 0, G_MAXLONG, FALSE, XA_ATOM, &type,
                &format, &n_atoms, &bytes_after,
                (unsigned char **) &atoms) == Success) || (type != None && type != XA_ATOM)) {  
    	if (format == 32) {
	    int n;
	    for (n = 0; n < n_atoms; n++) {
	        if(((long *)atoms)[n] == supportedAtom) {
                    isSupported = TRUE;
		    g_message ("True");
	            break;
	        }
            }
        }
    }	
    
    if (atoms) {
    	XFree (atoms);
    }
    return isSupported;
}

static void
wm_send_show_desktop (Atom atom, gboolean show, Display *dsp)
{
    XEvent event;
    event.xclient.type = ClientMessage;
    event.xclient.serial = 0;
    event.xclient.send_event = True;
    event.xclient.message_type = atom;
    event.xclient.window = DefaultRootWindow (dsp);
    event.xclient.format = 32;
    event.xclient.data.l[0] = show;
    XSendEvent (dsp, DefaultRootWindow(dsp), False, SubstructureRedirectMask | SubstructureNotifyMask, &event);
}

static void
do_window_actions (gboolean hide_all, gpointer data)
{
    gui *p = data;
    NetkScreen *screen;
    NetkWorkspace *workspace;
    NetkWindow *window;
    NetkWindow *active_window = NULL;
    
    GList *w = NULL;
    
    screen = netk_screen_get_default();
    workspace = netk_screen_get_active_workspace (screen);
    w = netk_screen_get_windows_stacked (screen);
    
    while (w != NULL) {
        window = w->data;
	if (!NETK_IS_WINDOW(window)) {
 	    w = w->next;
	    continue;
	}
        if (!netk_window_is_sticky (window)) {
            if (workspace == netk_window_get_workspace (window)) {
                if (hide_all) {
		    if (!netk_window_is_minimized (window)) {
	                netk_window_minimize (window);
		    }
	        } else {
		    if (netk_window_is_minimized) {
	                netk_window_unminimize (window);
			active_window=window;
	            }
                }
	    }
	 }
 	 w = w->next;   
    }
    if (active_window != NULL && !hide_all) {
        netk_window_activate (active_window);
    }
}

void
show_all_clicked (GtkWidget *button, GdkEventButton *ev, gpointer data)
{
    Atom net_showing_desktop;
    gui *plugin=data;
    gboolean wm;
    
    if (ev->button == 3) {
        plugin->function(button, ev, plugin->ctrl);
	return;
    }

    if (ev->button == 1) {
    	wm = True;
    } else {
        wm = False;
    }
    	
    if (plugin->alternate)
    	wm=!wm;

    if (wm) {
        net_showing_desktop = XInternAtom (GDK_DISPLAY(), "_NET_SHOWING_DESKTOP", False);
        if (wm_has_support(net_showing_desktop, GDK_DISPLAY())) {
	    if (plugin->swapCommands) {
    	        wm_send_show_desktop(net_showing_desktop, TRUE, GDK_DISPLAY());
	    } else {
    	        wm_send_show_desktop(net_showing_desktop, FALSE, GDK_DISPLAY());
	    }
	} else {
            plugin->swapCommands ? do_window_actions(TRUE, data) : do_window_actions(FALSE, data);
	}
    } else {
        plugin->swapCommands ? do_window_actions(TRUE, data) : do_window_actions(FALSE, data);
    }
}

void
hide_all_clicked (GtkWidget *button, GdkEventButton *ev, gpointer data)
{
    Atom net_showing_desktop=0;
    gui *plugin=data;
    gboolean wm;

    if (ev->button == 3) {
        plugin->function(button, ev, plugin->ctrl);
	return;
    }

    if (ev->button == 1) {
    	wm = True;
    } else {
        wm = False;
    }
    	
    if (plugin->alternate)
    	wm=!wm;

    if (wm) {
        net_showing_desktop = XInternAtom (GDK_DISPLAY(), "_NET_SHOWING_DESKTOP", False);
        if (wm_has_support(net_showing_desktop, GDK_DISPLAY())) {
	    if (plugin->swapCommands) {
    	        wm_send_show_desktop(net_showing_desktop, FALSE, GDK_DISPLAY());
	    } else {
    	        wm_send_show_desktop(net_showing_desktop, TRUE, GDK_DISPLAY());
	    }
	} else {
            plugin->swapCommands ? do_window_actions(FALSE, data) : do_window_actions(TRUE, data);
	}
    } else {
        plugin->swapCommands ? do_window_actions(FALSE, data) : do_window_actions (TRUE, data);
    }
}

static void
show_hide_clicked (GtkWidget *button, GdkEventButton *ev, gpointer data)
{
    gui *plugin = data;
    GdkPixbuf *tmp, *pb0, *pb1;
    Atom net_showing_desktop;
    gboolean wm;
    
    if (ev->button == 3) {
	plugin->function(button, ev, plugin->ctrl);
	return;
    }
    
    if (ev->button == 1) {
    	wm = True;
    } else {
        wm = False;
    }
    	
    if (plugin->alternate)
    	wm=!wm;

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

    if (wm) {
        net_showing_desktop = XInternAtom (GDK_DISPLAY(), "_NET_SHOWING_DESKTOP", False);
        if (wm_has_support(net_showing_desktop, GDK_DISPLAY())) {
            if (plugin->hide) { 
                if (plugin->swapCommands) {
                    wm_send_show_desktop(net_showing_desktop, FALSE, GDK_DISPLAY());
                } else {
                    wm_send_show_desktop(net_showing_desktop, TRUE, GDK_DISPLAY());
                }
            } else {	
                if (plugin->swapCommands) {
                    wm_send_show_desktop(net_showing_desktop, TRUE, GDK_DISPLAY());
                } else {
                    wm_send_show_desktop(net_showing_desktop, FALSE, GDK_DISPLAY());
                }
            }
        } else {
            if (plugin->hide) {
                plugin->swapCommands ? do_window_actions(FALSE, data) : do_window_actions (TRUE, data);
	    } else {
                plugin->swapCommands ? do_window_actions(TRUE, data) : do_window_actions(FALSE, data);
	    }
        }
    } else {
        if (plugin->hide) {
            plugin->swapCommands ? do_window_actions(FALSE, data) : do_window_actions (TRUE, data);
	} else {
            plugin->swapCommands ? do_window_actions(TRUE, data) : do_window_actions(FALSE, data);
	}
    }
    
    if (plugin->hide) {
        xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON(plugin->show_hide), pb1);
    } else {
        xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON(plugin->show_hide), pb0);
    }
    plugin->hide=!plugin->hide;
}

static gui *
gui_new ()
{
    gui *plugin;
    plugin = g_new(gui, 1);
    plugin->swapCommands = FALSE;
    plugin->alternate = FALSE;
    plugin->lessSpace = FALSE;
    plugin->oneButton = FALSE;
    plugin->hide = TRUE;
    plugin->IconSize = ICONSIZETINY;
    plugin->ebox = gtk_event_box_new();
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
    xmlChar *space;
    xmlChar *alternate;
    gui *plugin = ctrl->data;
    
    oneb = xmlGetProp (parent, (const xmlChar *) "oneButton");
    
    if (oneb) {
        if (!strcmp (oneb, "0")) {
            plugin->oneButton = TRUE;
        }
    } else {
            plugin->oneButton = FALSE;
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
    
    alternate = xmlGetProp (parent, (const xmlChar *) "alternate");
    
    if (alternate) {
        if (!strcmp (alternate, "0")) {
            plugin->alternate = TRUE;
        }
    } else {
            plugin->alternate = FALSE;
    }

    g_free (swap);
    g_free (alternate);
    g_free (space);
    g_free (oneb);
    plugin_recreate_gui (plugin);
}

static void
plugin_write_config (Control *ctrl, xmlNodePtr parent)
{   
    gui *plugin = ctrl->data;
    char swap[2];
    char alternate[2];
    char size[2];
    char oneb[2];
    
    if (plugin->swapCommands) {
        g_snprintf (swap, 2, "%i", 0);
    } else {
        g_snprintf (swap, 2, "%i", 1);
    }
    
    if (plugin->alternate) {
        g_snprintf (alternate, 2, "%i", 0);
    } else {
        g_snprintf (alternate, 2, "%i", 1);
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
    xmlSetProp (parent, (const xmlChar *) "alternate", alternate);
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
    gboolean alternate;

    alternate = gtk_toggle_button_get_active (cb);
    plugin->alternate = alternate;
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
    GtkWidget *vbox, *optionbox, *cb1, *cb2, *cb3, *cb4,*frame;
    
    gchar *tips[] = {
        N_("Enable this options, if your panel is located on top or on the right."),
        N_("Normally, the first mouse button is connected to the windowmanager showdesktop feature, if the WM supports it, and the middle mouse button is connected to the plugin internal showdesktop feature. You can reverse it with this option."),
        N_("If this options is enabled, the plugin consumes less space on the panel."),
        N_("one button only"),
    };

    optiontooltips = gtk_tooltips_new ();

    vbox = gtk_vbox_new (False, 5);
    
    cb1 = gtk_check_button_new_with_label N_("swap commands");
    gtk_tooltips_set_tip (optiontooltips, cb1, tips[0], NULL);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb1), plugin->swapCommands);
    g_signal_connect (cb1, "toggled", G_CALLBACK (plugin_cb1_changed), plugin);
    
    cb2 = gtk_check_button_new_with_label N_("alternate button mode");
    gtk_tooltips_set_tip (optiontooltips, cb2, tips[1], NULL);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb2), plugin->alternate);
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
