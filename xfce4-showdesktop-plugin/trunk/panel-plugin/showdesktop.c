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
#include <libxfcegui4/netk-class-group.h>

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
    GtkWidget	*app_show;
    GtkWidget	*app_close;
    GtkWidget   *save_state1;
    GtkWidget   *save_state2;
    GtkWidget   *mainbox;
    GtkWidget   *addbox;
    GtkWidget   *savebox;
    GtkWidget   *base;
    GtkWidget	*ebox;
    gint        orientation;
    gint        IconSize;
    gboolean    swapCommands;
    gboolean    showTooltips;
    gboolean    lessSpace;
    gboolean    showAddButtons;
    gboolean    showMainButtons;
    gboolean    showSaveButtons;
    GList       *windows;
    GList	*SaveState1Min;
    GList	*SaveState1Unmin;
    NetkWindow	*SaveState1Act;
    NetkWorkspace *SaveState1Workspace;
    GList	*SaveState2Min;
    GList	*SaveState2Unmin;
    NetkWindow	*SaveState2Act;
    NetkWorkspace *SaveState2Workspace;
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
	    4... show all windows of the current class group
	    5... hide all windows of the current class group
	    6... close all windows except the current class group
	    7... close all windows of the current class group
	 */

static void
do_window_actions (int type, gpointer data)
{
    gui *p = data;
    NetkScreen *screen;
    NetkWorkspace *workspace;
    NetkWindow *window;
    NetkWindow *active_window = NULL;
    NetkClassGroup *classgroup;
    NetkWindow *win;
    
    GList *w = NULL;
    GList *tmpwins = NULL;
    GList *classwins = NULL;
    
    screen = netk_screen_get_default();
    workspace = netk_screen_get_active_workspace (screen);
    if (type == 2 || type == 3) {
        w = p->windows;
	if (type == 5) {
            active_window = netk_screen_get_active_window (screen);
	}
    } else if (type == 0 || type == 1) {
        w = netk_screen_get_windows_stacked (screen);
    } else if (type == 4 || type == 5 || type == 6 || type == 7) {
        active_window = netk_screen_get_active_window (screen);
	if (active_window) {
	    classgroup = netk_window_get_class_group (active_window);
	    if (classgroup) {
	        tmpwins = netk_screen_get_windows_stacked (screen);
		classwins = netk_class_group_get_windows (classgroup);
		if (tmpwins && classwins && type == 4 || type == 6) {
		    while (tmpwins != NULL) {
		        win = tmpwins->data;
		        if (workspace != netk_window_get_workspace(win)) {
		            tmpwins = tmpwins->next;
		            continue;
		        }
		        if (!g_list_find(classwins, win)) {
		            w = g_list_append (w, win);
			} else {
			    if (netk_window_is_minimized(win) && type == 4) {
			        netk_window_unminimize(win);
			    }
			}
		        tmpwins = tmpwins->next;
		    }
		} else if (classwins && type == 5 || type == 7) {
		    while (classwins != NULL) {
		        win = classwins->data;
		        if (workspace == netk_window_get_workspace(win)) {
		            w = g_list_append (w, win);
		        }
			classwins = classwins->next;
		    }
                }
	    }
	}       
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
                if (type == 0 || type == 2 || type == 4 || type == 5) {
		    if (!netk_window_is_minimized (window)) {
	                netk_window_minimize (window);
		        p->windows = g_list_append (p->windows, window);
		    }
	        } else if (type == 1 || type == 3) {
		    if (netk_window_is_minimized (window)) {
	                netk_window_unminimize (window);
		        p->windows = g_list_append (p->windows, window);
	            }
	        } else {
		    netk_window_close (window);
		}
	    }
	 }
 	 w = w->next;   
    }
    if (active_window != NULL && (type == 1 || type == 3 || type == 4 || type == 6)) {
        netk_window_activate (active_window);
    }
}

static void
RestoreSaveState (int state, gpointer data)
{
    gui *plugin = data;
    GList *min = NULL;
    GList *unmin = NULL;
    GList *windows = NULL;
    NetkScreen *screen;
    NetkWorkspace *ws;
    NetkWindow *win, *actwin;
    
    switch (state) {
    	case 1:
	    min = plugin->SaveState1Min;
	    unmin = plugin->SaveState1Unmin;
	    actwin = plugin->SaveState1Act;
	    ws = plugin->SaveState1Workspace;
	    break;
	case 2:
	    min = plugin->SaveState2Min;
	    unmin = plugin->SaveState2Unmin;
	    actwin = plugin->SaveState2Act;
	    ws = plugin->SaveState2Workspace;
	    break;
    }
    
    screen = netk_screen_get_default();

    if (ws != netk_screen_get_active_workspace(screen)) {
    	if (NETK_IS_WORKSPACE(ws)) {
	    netk_workspace_activate (ws);
	} else {
	    return;
	}
    }
	    
    windows = netk_screen_get_windows_stacked (screen);

    while (windows!=NULL) {
	win = windows->data;
	
	if (!netk_window_is_sticky(win) && (ws == netk_window_get_workspace (win))) {
	    if (g_list_find(min, win)) {
	        netk_window_minimize (win);
	    } else if (g_list_find(unmin, win)) {
	        netk_window_unminimize (win);
	    } else {
	        netk_window_close (win);
	    }
	}

	windows = windows->next;
    }

    if (NETK_IS_WINDOW(actwin) && ws == netk_window_get_workspace (actwin))
        netk_window_activate (actwin);
}

static void
SetSaveState (int state, gpointer data)
{
    gui *plugin = data;
    GList *min = NULL;
    GList *unmin = NULL;
    GList *windows = NULL;
    NetkScreen *screen;
    NetkWorkspace *ws;
    NetkWindow *win, *actwin;

    screen = netk_screen_get_default();
    ws = netk_screen_get_active_workspace(screen);
    windows = netk_screen_get_windows_stacked(screen);
    
    actwin=netk_screen_get_active_window(screen);

    while (windows != NULL) {
    	win = windows->data;
        if (netk_window_is_sticky(win) || ws != netk_window_get_workspace (win)) {
	    windows=windows->next;
	    continue;
	}
	if (netk_window_is_minimized (win)) {
	    min = g_list_append (min, win);
	} else {
	    unmin = g_list_append (unmin, win);
	}

	windows=windows->next;
    }
    
    switch (state) {
    	case 1:
	    plugin->SaveState1Min = min;
	    plugin->SaveState1Unmin = unmin;
	    plugin->SaveState1Act = actwin;
	    plugin->SaveState1Workspace = ws;
	    break;
	case 2:
	    plugin->SaveState2Min = min;
	    plugin->SaveState2Unmin = unmin;
	    plugin->SaveState2Act = actwin;
	    plugin->SaveState2Workspace = ws;
	    break;
    }
}
    
static void
app_close_clicked (GtkWidget *button, GdkEventButton *ev, gpointer data)
{
    gui *plugin=data;
    if (ev->button == 1) {
        do_window_actions (6, data);
    } else if (ev->button == 2) {
        do_window_actions (7, data);
    } else if (ev->button == 3) {
        plugin->function(button, ev, plugin->ctrl);
    }
}

static void
app_show_clicked (GtkWidget *button, GdkEventButton *ev, gpointer data)
{
    gui *plugin=data;
    if (ev->button == 1) {
        do_window_actions (4, data);
    } else if (ev->button == 2) {
        do_window_actions (5, data);
    } else if (ev->button == 3) {
        plugin->function(button, ev, plugin->ctrl);
    }
}

static void
show_all_clicked (GtkWidget *button, GdkEventButton *ev, gpointer data)
{
    gui *plugin=data;
    if (ev->button == 1) {
        do_window_actions(1, data);
    } else if (ev->button == 2) {
        do_window_actions(3, data);
    } else if (ev->button == 3) {
	plugin->function(button, ev, plugin->ctrl);
    }
}

static void
hide_all_clicked (GtkWidget *button, GdkEventButton *ev, gpointer data)
{
    gui *plugin=data;
    if (ev->button == 1) {
        do_window_actions(0, data);
    } else if (ev->button == 2) {
        do_window_actions(2, data);
    } else if (ev->button == 3) {
	plugin->function(button, ev, plugin->ctrl);
    }
}

static void
save_state1_clicked (GtkWidget *button, GdkEventButton *ev, gpointer data)
{
    gui *plugin=data;
    if (ev->button == 1) {
        SetSaveState(1, data);
    } else if (ev->button == 2) {
        RestoreSaveState(1, data);
    } else if (ev->button == 3) {
	plugin->function(button, ev, plugin->ctrl);
    }
}

static void
save_state2_clicked (GtkWidget *button, GdkEventButton *ev, gpointer data)
{
    gui *plugin=data;
    if (ev->button == 1) {
        SetSaveState(2, data);
    } else if (ev->button == 2) {
        RestoreSaveState(2, data);
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
    plugin->showSaveButtons = FALSE;
    plugin->showAddButtons = FALSE;
    plugin->showMainButtons = TRUE;
    plugin->IconSize = ICONSIZETINY;
    plugin->ebox = gtk_event_box_new();
    plugin->windows = NULL;
    gtk_widget_show (plugin->ebox);
    
    plugin->base = gtk_hbox_new(FALSE, 0);
    gtk_widget_show (plugin->base);
    
    plugin->mainbox = gtk_hbox_new(FALSE, 0);
    gtk_widget_show (plugin->mainbox);

    plugin->addbox = gtk_hbox_new(FALSE, 0);
    gtk_widget_show (plugin->addbox);

    plugin->savebox = gtk_hbox_new(FALSE, 0);
    gtk_widget_show (plugin->savebox);

    plugin->show_all = xfce_iconbutton_new ();
    plugin->hide_all = xfce_iconbutton_new ();
    plugin->app_show = xfce_iconbutton_new ();
    plugin->app_close = xfce_iconbutton_new ();
    plugin->save_state1 = xfce_iconbutton_new ();
    plugin->save_state2 = xfce_iconbutton_new ();

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
        gtk_tooltips_set_tip (tooltips, plugin->app_show,
	      N_("Button 1: show only windows of the current class group\nButton 2: Hide all windows of the current class group"), NULL);
	gtk_tooltips_set_tip (tooltips, plugin->app_close,
	      N_("Button1: Close all windows except the current class group\nButton2: Close all windows of the current class group"), NULL);
	gtk_tooltips_set_tip (tooltips, plugin->save_state1,
	      N_("Button 1: save window state 1\nButton 2: restore window state 1"), NULL);
	gtk_tooltips_set_tip (tooltips, plugin->save_state2,
	      N_("Button 1: save window state 2\nButton 2: restore window state 2"), NULL);
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

    if (plugin->showAddButtons) {
        tmp = gtk_widget_render_icon (plugin->ebox, GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU, NULL);
        pb2 = gdk_pixbuf_scale_simple (tmp, plugin->IconSize, plugin->IconSize, GDK_INTERP_BILINEAR);
        g_object_unref (tmp);
        xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON(plugin->app_show), pb2);
	
        tmp = gtk_widget_render_icon (plugin->ebox, GTK_STOCK_STOP, GTK_ICON_SIZE_MENU, NULL);
        pb3 = gdk_pixbuf_scale_simple (tmp, plugin->IconSize, plugin->IconSize, GDK_INTERP_BILINEAR);
        g_object_unref (tmp);
        xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON(plugin->app_close), pb3);
    }

    if (plugin->showSaveButtons) {
        tmp = gtk_widget_render_icon (plugin->ebox, GTK_STOCK_SAVE, GTK_ICON_SIZE_MENU, NULL);
        pb4 = gdk_pixbuf_scale_simple (tmp, plugin->IconSize, plugin->IconSize, GDK_INTERP_BILINEAR);
        g_object_unref (tmp);
        xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON(plugin->save_state1), pb4);
        xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON(plugin->save_state2), pb4);
    }
    
    xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON(plugin->show_all), pb0);
    xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON(plugin->hide_all), pb1);
} 
   
static void
plugin_recreate_gui (gui *plugin)
{
    GdkPixbuf *tmp, *pb0, *pb1, *pb2, *pb3, *pb4;

    gtk_widget_destroy (plugin->base);

    plugin->show_all = xfce_iconbutton_new ();
    plugin->hide_all = xfce_iconbutton_new ();

    plugin->save_state1 = xfce_iconbutton_new ();
    g_signal_connect (plugin->save_state1, "button_press_event", G_CALLBACK(save_state1_clicked), plugin);
    tmp = gtk_widget_render_icon (plugin->ebox, GTK_STOCK_SAVE, GTK_ICON_SIZE_MENU, NULL);
    pb4 = gdk_pixbuf_scale_simple (tmp, plugin->IconSize, plugin->IconSize, GDK_INTERP_BILINEAR);
    g_object_unref (tmp);
    xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON(plugin->save_state1), pb4);
    gtk_button_set_relief (GTK_BUTTON (plugin->save_state1), GTK_RELIEF_NONE);

    plugin->save_state2 = xfce_iconbutton_new ();
    g_signal_connect (plugin->save_state2, "button_press_event", G_CALLBACK(save_state2_clicked), plugin);
    xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON(plugin->save_state2), pb4);
    gtk_button_set_relief (GTK_BUTTON (plugin->save_state2), GTK_RELIEF_NONE);
   
    plugin->app_show = xfce_iconbutton_new ();
    g_signal_connect (plugin->app_show, "button_press_event", G_CALLBACK(app_show_clicked), plugin);
    tmp = gtk_widget_render_icon (plugin->ebox, GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU, NULL);
    pb2 = gdk_pixbuf_scale_simple (tmp, plugin->IconSize, plugin->IconSize, GDK_INTERP_BILINEAR);
    g_object_unref (tmp);
    xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON(plugin->app_show), pb2);
    gtk_button_set_relief (GTK_BUTTON (plugin->app_show), GTK_RELIEF_NONE);

    plugin->app_close = xfce_iconbutton_new ();
    g_signal_connect (plugin->app_close, "button_press_event", G_CALLBACK(app_close_clicked), plugin);
    tmp = gtk_widget_render_icon (plugin->ebox, GTK_STOCK_STOP, GTK_ICON_SIZE_MENU, NULL);
    pb3 = gdk_pixbuf_scale_simple (tmp, plugin->IconSize, plugin->IconSize, GDK_INTERP_BILINEAR);
    g_object_unref (tmp);
    xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON(plugin->app_close), pb3);
    gtk_button_set_relief (GTK_BUTTON (plugin->app_close), GTK_RELIEF_NONE);

    if (plugin->orientation == HORIZONTAL) {
    	tmp = gtk_widget_render_icon (plugin->ebox, GTK_STOCK_GO_UP, GTK_ICON_SIZE_MENU, NULL);
	pb0 = gdk_pixbuf_scale_simple (tmp, plugin->IconSize, plugin->IconSize, GDK_INTERP_BILINEAR);
	g_object_unref (tmp);
	
        tmp = gtk_widget_render_icon (plugin->ebox, GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_MENU, NULL);
	pb1 = gdk_pixbuf_scale_simple (tmp, plugin->IconSize, plugin->IconSize, GDK_INTERP_BILINEAR);
	g_object_unref (tmp);

        if (plugin->lessSpace) {
	    plugin->base = gtk_hbox_new (FALSE, 0);
	    plugin->mainbox = gtk_vbox_new (FALSE, 0);
	    plugin->addbox = gtk_vbox_new (FALSE, 0);
	    plugin->savebox = gtk_vbox_new (FALSE, 0);
            if (plugin->showAddButtons) {
                gtk_widget_set_size_request (plugin->app_show, plugin->IconSize * 2, plugin->IconSize);
                gtk_widget_set_size_request (plugin->app_close, plugin->IconSize * 2, plugin->IconSize);
            }
	    
            if (plugin->showSaveButtons) {
                gtk_widget_set_size_request (plugin->save_state1, plugin->IconSize * 2, plugin->IconSize);
                gtk_widget_set_size_request (plugin->save_state2, plugin->IconSize * 2, plugin->IconSize);
            }
            gtk_widget_set_size_request (plugin->show_all, plugin->IconSize * 2, plugin->IconSize);
            gtk_widget_set_size_request (plugin->hide_all, plugin->IconSize * 2, plugin->IconSize);
	} else {
	   plugin->base = gtk_hbox_new (FALSE, 0);
	   plugin->mainbox = gtk_hbox_new (FALSE, 0);
	   plugin->addbox = gtk_hbox_new (FALSE, 0);
	   plugin->savebox = gtk_hbox_new (FALSE, 0);
	   if (plugin->showAddButtons) {
	       gtk_widget_set_size_request (plugin->app_show, plugin->IconSize * 2, -1);
	       gtk_widget_set_size_request (plugin->app_close, plugin->IconSize * 2, -1);
	   }

	   if (plugin->showSaveButtons) {
	       gtk_widget_set_size_request (plugin->save_state1, plugin->IconSize * 2, -1);
	       gtk_widget_set_size_request (plugin->save_state2, plugin->IconSize * 2, -1);
	   }
	   gtk_widget_set_size_request (plugin->show_all, plugin->IconSize * 2, -1);
	   gtk_widget_set_size_request (plugin->hide_all, plugin->IconSize * 2, -1);
	}

        gtk_container_add (GTK_CONTAINER(plugin->mainbox), plugin->show_all);
        gtk_container_add (GTK_CONTAINER(plugin->mainbox), plugin->hide_all);
    
        gtk_container_add (GTK_CONTAINER(plugin->addbox), plugin->app_show);
        gtk_container_add (GTK_CONTAINER(plugin->addbox), plugin->app_close);
	
        gtk_container_add (GTK_CONTAINER(plugin->savebox), plugin->save_state1);
        gtk_container_add (GTK_CONTAINER(plugin->savebox), plugin->save_state2);
         
    } else {
        tmp = gtk_widget_render_icon (plugin->ebox, GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_MENU, NULL);
	pb0 = gdk_pixbuf_scale_simple (tmp, plugin->IconSize, plugin->IconSize, GDK_INTERP_BILINEAR);
	g_object_unref (tmp);

        tmp = gtk_widget_render_icon (plugin->ebox, GTK_STOCK_GO_BACK, GTK_ICON_SIZE_MENU, NULL);
	pb1 = gdk_pixbuf_scale_simple (tmp, plugin->IconSize, plugin->IconSize, GDK_INTERP_BILINEAR);
	g_object_unref (tmp);
	
        if (plugin->lessSpace) {
	    plugin->base = gtk_vbox_new (FALSE, 0);
	    plugin->mainbox = gtk_hbox_new (FALSE, 0);
	    plugin->addbox = gtk_hbox_new (FALSE, 0);
	    plugin->savebox = gtk_hbox_new (FALSE, 0);
	    if (plugin->showAddButtons) {
	        gtk_widget_set_size_request (plugin->app_show, plugin->IconSize, plugin->IconSize * 2);
	        gtk_widget_set_size_request (plugin->app_close, plugin->IconSize, plugin->IconSize * 2);
	    }

	    if (plugin->showSaveButtons) {
	        gtk_widget_set_size_request (plugin->save_state1, plugin->IconSize, plugin->IconSize * 2);
	        gtk_widget_set_size_request (plugin->save_state2, plugin->IconSize, plugin->IconSize * 2);
	    }
	    gtk_widget_set_size_request (plugin->show_all, plugin->IconSize, plugin->IconSize * 2);
	    gtk_widget_set_size_request (plugin->hide_all, plugin->IconSize, plugin->IconSize * 2);
	} else {
	    plugin->base = gtk_vbox_new (FALSE, 0);
	    plugin->mainbox = gtk_vbox_new (FALSE, 0);
	    plugin->addbox = gtk_vbox_new (FALSE, 0);
	    plugin->savebox = gtk_vbox_new (FALSE, 0);
	    if (plugin->showAddButtons) {
	        gtk_widget_set_size_request (plugin->app_show, -1, plugin->IconSize * 2);
	        gtk_widget_set_size_request (plugin->app_close, -1, plugin->IconSize * 2);
	    }

	    if (plugin->showSaveButtons) {
	        gtk_widget_set_size_request (plugin->save_state1, -1, plugin->IconSize * 2);
	        gtk_widget_set_size_request (plugin->save_state2, -1, plugin->IconSize * 2);
	    }
	    gtk_widget_set_size_request (plugin->show_all, -1, plugin->IconSize * 2);
	    gtk_widget_set_size_request (plugin->show_all, -1, plugin->IconSize * 2);
	    gtk_widget_set_size_request (plugin->hide_all, -1, plugin->IconSize * 2);
	}

        gtk_container_add (GTK_CONTAINER(plugin->mainbox), plugin->hide_all);
        gtk_container_add (GTK_CONTAINER(plugin->mainbox), plugin->show_all);
    
        gtk_container_add (GTK_CONTAINER(plugin->addbox), plugin->app_close);
        gtk_container_add (GTK_CONTAINER(plugin->addbox), plugin->app_show);
	
        gtk_container_add (GTK_CONTAINER(plugin->savebox), plugin->save_state1);
        gtk_container_add (GTK_CONTAINER(plugin->savebox), plugin->save_state2);
    }	
    
    xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON(plugin->show_all), pb0);
    xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON(plugin->hide_all), pb1);

    gtk_button_set_relief (GTK_BUTTON (plugin->show_all), GTK_RELIEF_NONE);
    gtk_button_set_relief (GTK_BUTTON (plugin->hide_all), GTK_RELIEF_NONE);
    
    gtk_container_add (GTK_CONTAINER(plugin->base), plugin->mainbox);
    gtk_container_add (GTK_CONTAINER(plugin->base), plugin->addbox);
    gtk_container_add (GTK_CONTAINER(plugin->base), plugin->savebox);
 
    gtk_container_add (GTK_CONTAINER(plugin->ebox), plugin->base);
    gtk_widget_show_all (plugin->base);

    if (!plugin->showAddButtons)
        gtk_widget_hide (plugin->addbox);
        
    if (!plugin->showMainButtons)
        gtk_widget_hide (plugin->mainbox);
	
    if (!plugin->showSaveButtons)
        gtk_widget_hide (plugin->savebox);
        
    if (plugin->swapCommands) {
        g_signal_connect (plugin->show_all, "button_press_event", G_CALLBACK(hide_all_clicked), plugin);
        g_signal_connect (plugin->hide_all, "button_press_event", G_CALLBACK(show_all_clicked), plugin);
    } else {
        g_signal_connect (plugin->show_all, "button_press_event", G_CALLBACK(show_all_clicked), plugin);
        g_signal_connect (plugin->hide_all, "button_press_event", G_CALLBACK(hide_all_clicked), plugin);
    }

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
    xmlChar *tool;
    xmlChar *space;
    xmlChar *add;
    xmlChar *save;
    xmlChar *main;
    gui *plugin = ctrl->data;
    
    main = xmlGetProp (parent, (const xmlChar *) "showMainButtons");

    if (main) {
        if (!strcmp (main, "1")) {
            plugin->showMainButtons = FALSE;
	}
    } else {
            plugin->showMainButtons = TRUE;
    }
 
    add = xmlGetProp (parent, (const xmlChar *) "showAddButtons");

    if (add) {
        if (!strcmp (add, "0")) {
            plugin->showAddButtons = TRUE;
	}
    } else {
            plugin->showAddButtons = FALSE;
    }
    
    save = xmlGetProp (parent, (const xmlChar *) "showSaveButtons");

    if (save) {
        if (!strcmp (save, "0")) {
            plugin->showSaveButtons = TRUE;
	}
    } else {
            plugin->showSaveButtons = FALSE;
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
    g_free (add);
    g_free (save);
    g_free (main);
    plugin_recreate_gui (plugin);
}

static void
plugin_write_config (Control *ctrl, xmlNodePtr parent)
{   
    gui *plugin = ctrl->data;
    char swap[2];
    char tool[2];
    char size[2];
    char add[2];
    char save[2];
    char main[2];
    
    if (plugin->showMainButtons) {
        g_snprintf (main, 2, "%i", 0);
    } else {
        g_snprintf (main, 2, "%i", 1);
    }
 
    if (plugin->showAddButtons) {
        g_snprintf (add, 2, "%i", 0);
    } else {
        g_snprintf (add, 2, "%i", 1);
    }

    if (plugin->showSaveButtons) {
        g_snprintf (save, 2, "%i", 0);
    } else {
        g_snprintf (save, 2, "%i", 1);
    }
 
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

    xmlSetProp (parent, (const xmlChar *) "showMainButtons", main);
    xmlSetProp (parent, (const xmlChar *) "showAddButtons", add);
    xmlSetProp (parent, (const xmlChar *) "showSaveButtons", save);
    xmlSetProp (parent, (const xmlChar *) "swapCommands", swap);
    xmlSetProp (parent, (const xmlChar *) "showTooltips", tool);
    xmlSetProp (parent, (const xmlChar *) "lessSpace", size);
}

static void
plugin_set_orientation (Control *ctrl, int orientation)
{
    gui *plugin = ctrl->data;
    plugin->orientation = orientation;
    plugin_recreate_gui (plugin);
}

static void
plugin_infomsg () {
    xfce_info N_("At least one button group must be enabled!");
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
plugin_cb4_changed (GtkToggleButton *cb, gui *plugin)
{
    gboolean showMainButtons;

    showMainButtons = gtk_toggle_button_get_active (cb);

    if (showMainButtons) {
	plugin->showMainButtons = TRUE;
    } else {
        if (!plugin->showAddButtons && !plugin->showSaveButtons) {
	    plugin->showMainButtons = TRUE;
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb), TRUE);
	    plugin_infomsg();
	} else {
            plugin->showMainButtons = FALSE;
	}
    }
    plugin_recreate_gui (plugin);
}

static void
plugin_cb5_changed (GtkToggleButton *cb, gui *plugin)
{
    gboolean showAddButtons;

    showAddButtons = gtk_toggle_button_get_active (cb);

    if (showAddButtons) {
	plugin->showAddButtons = TRUE;
    } else {
        if (!plugin->showMainButtons && !plugin->showSaveButtons) {
	    plugin->showAddButtons = TRUE;
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb), TRUE);
	    plugin_infomsg();
        } else {
            plugin->showAddButtons = FALSE;
	}
    }
    plugin_recreate_gui (plugin);
}

static void
plugin_cb6_changed (GtkToggleButton *cb, gui *plugin)
{
    gboolean showSaveButtons;

    showSaveButtons = gtk_toggle_button_get_active (cb);

    if (showSaveButtons) {
	plugin->showSaveButtons = TRUE;
    } else {
        if (!plugin->showAddButtons && !plugin->showMainButtons) {
	    plugin->showSaveButtons = TRUE;
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb), TRUE);
	    plugin_infomsg();
        } else {
            plugin->showSaveButtons = FALSE;
	}
    }
    plugin_recreate_gui (plugin);
}

static void
plugin_create_options (Control *ctrl, GtkContainer *con, GtkWidget *done)
{
    gui *plugin = ctrl->data;
    GtkWidget *vbox, *rbbox, *optionbox, *cb1, *cb2, *cb3, *cb4, *cb5, *cb6, *frame1, *frame2;
    
    optiontooltips = gtk_tooltips_new ();

    gchar *tips[] = {
        N_("Enable this options, if your panel is located on top or on the right."),
        N_("Enable this option, if you want tooltips on all command buttons."),
        N_("If this options is enabled, the plugin consumes less space on the panel."),
        N_("Display the standart button group to minimize or unminimize all windows on the current workspace."),
        N_("This button group allows you to act with class windows, for example to minimize all Gimp windows on the current workspace."),
        N_("Enable this button group to be able to save and restore the windows on the current workspace.")
    };
    
    vbox = gtk_vbox_new (1, 1);

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

    optionbox = gtk_vbox_new (1, 1);
    frame2 = gtk_frame_new N_("Options");
 
    rbbox = gtk_vbox_new (1, 1);
    frame1 = gtk_frame_new N_("Buttongroups");

    cb4 = gtk_check_button_new_with_label N_("show/hide all");
    gtk_tooltips_set_tip (optiontooltips, cb4, tips[3], NULL);
    g_signal_connect (cb4, "toggled", G_CALLBACK (plugin_cb4_changed), plugin);

    cb5 = gtk_check_button_new_with_label N_("class group");
    gtk_tooltips_set_tip (optiontooltips, cb5, tips[4], NULL);
    g_signal_connect (cb5, "toggled", G_CALLBACK (plugin_cb5_changed), plugin);

    cb6 = gtk_check_button_new_with_label N_("save state");
    gtk_tooltips_set_tip (optiontooltips, cb6, tips[5], NULL);
    g_signal_connect (cb6, "toggled", G_CALLBACK (plugin_cb6_changed), plugin);
    
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb4), plugin->showMainButtons);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb5), plugin->showAddButtons);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb6), plugin->showSaveButtons);
    	
    gtk_container_add (con, vbox);
    gtk_container_add (GTK_CONTAINER(optionbox), cb1);
    gtk_container_add (GTK_CONTAINER(optionbox), cb2);
    gtk_container_add (GTK_CONTAINER(optionbox), cb3);
    gtk_container_add (GTK_CONTAINER(rbbox), cb4);
    gtk_container_add (GTK_CONTAINER(rbbox), cb5);
    gtk_container_add (GTK_CONTAINER(rbbox), cb6);
    gtk_container_add (GTK_CONTAINER(frame1), rbbox);
    gtk_container_add (GTK_CONTAINER(frame2), optionbox);
    gtk_container_add (GTK_CONTAINER(vbox), frame1);
    gtk_container_add (GTK_CONTAINER(vbox), frame2);
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
