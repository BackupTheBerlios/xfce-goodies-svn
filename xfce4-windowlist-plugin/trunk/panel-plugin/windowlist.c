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
#include <libxfcegui4/netk-screen.h>
#include <panel/plugins.h>
#include <panel/xfce.h>

// }}}

// struct {{{

typedef struct
{
    GtkWidget	    *ebox;
    GtkWidget	    *windowlist;
    NetkScreen      *screen;
    int		    ScreenCallbackId;
    gboolean	    includeAll;
} gui;

static GtkTooltips *tooltips = NULL;

// }}}

// all functions {{{

static void
plugin_activate_workspace (GtkWidget *widget, gpointer data)
{
    NetkWorkspace *ws = data;
    netk_workspace_activate (ws);
}

static void
plugin_activate_window (GtkWidget *widget, gpointer data)
{
    NetkWindow *win = data;
    NetkScreen *screen;
    NetkWorkspace *ws;
    
    screen = netk_screen_get_default ();
    ws = netk_window_get_workspace (win);

    if (ws != netk_screen_get_active_workspace (screen)) {
        netk_workspace_activate (ws);
    }
	
    netk_window_activate (win);
}

static void
plugin_stick_window (GtkWidget *widget, gpointer data)
{
    NetkWindow *win = data;
    netk_window_stick (win);
}

static void
plugin_unstick_window (GtkWidget *widget, gpointer data)
{
    NetkWindow *win = data;
    netk_window_unstick (win);
}

static void
plugin_minimize_window (GtkWidget *widget, gpointer data)
{
    NetkWindow *win = data;
    netk_window_minimize (win);
}
	
static void
plugin_unminimize_window (GtkWidget *widget, gpointer data)
{
    NetkWindow *win = data;
    netk_window_unminimize (win);
}

static void
plugin_maximize_window (GtkWidget *widget, gpointer data)
{
    NetkWindow *win = data;
    netk_window_maximize (win);
}
	
static void
plugin_unmaximize_window (GtkWidget *widget, gpointer data)
{
    NetkWindow *win = data;
    netk_window_unmaximize (win);
}

static void
plugin_shade_window (GtkWidget *widget, gpointer data)
{
    NetkWindow *win = data;
    netk_window_shade (win);
}
	
static void
plugin_unshade_window (GtkWidget *widget, gpointer data)
{
    NetkWindow *win = data;
    netk_window_unshade (win);
}

static void
plugin_close_window (GtkWidget *widget, gpointer data)
{
    NetkWindow *win = data;
    netk_window_close (win);
}

static GtkWidget *
plugin_create_action_menu (NetkWindow *win)
{
    GtkWidget *menu = NULL;
    GtkWidget *item, *image;
    const char *wname = NULL;
    GString *label;

    wname = netk_window_get_name (win);
    label = g_string_new (wname);

    if (label->len >= 20) {
        g_string_truncate (label, 20);
        g_string_append (label, " ...");
    }

    menu = gtk_menu_new ();

    item = gtk_menu_item_new_with_label (label->str);

    gtk_menu_shell_append (GTK_MENU_SHELL(menu), item);
    gtk_widget_set_sensitive (item, FALSE);
    g_signal_connect (item, "activate", G_CALLBACK(plugin_activate_window), win);

    g_string_free (label, TRUE);

    item = gtk_separator_menu_item_new ();
    gtk_menu_shell_append (GTK_MENU_SHELL(menu), item);

    if (netk_window_is_maximized(win)) {
        item = gtk_image_menu_item_new_with_label ("Unmaximize");
        image = gtk_image_new_from_stock (GTK_STOCK_ZOOM_OUT, GTK_ICON_SIZE_MENU);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(item), image);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect (item, "activate", G_CALLBACK(plugin_unmaximize_window), win);
    } else {
        item = gtk_image_menu_item_new_with_label ("Maximize");
        image = gtk_image_new_from_stock (GTK_STOCK_ZOOM_100, GTK_ICON_SIZE_MENU);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(item), image);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect (item, "activate", G_CALLBACK(plugin_maximize_window), win);
    }

    if (netk_window_is_minimized(win)) {
        item = gtk_image_menu_item_new_with_label ("Show");
        image = gtk_image_new_from_stock (GTK_STOCK_REDO, GTK_ICON_SIZE_MENU);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(item), image);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect (item, "activate", G_CALLBACK(plugin_unminimize_window), win);
    } else {
        item = gtk_image_menu_item_new_with_label ("Hide");
        image = gtk_image_new_from_stock (GTK_STOCK_UNDO, GTK_ICON_SIZE_MENU);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(item), image);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect (item, "activate", G_CALLBACK(plugin_minimize_window), win);
    }
    
    if (netk_window_is_shaded(win)) {
        item = gtk_image_menu_item_new_with_label ("Unshade");
        image = gtk_image_new_from_stock (GTK_STOCK_GOTO_BOTTOM, GTK_ICON_SIZE_MENU);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(item), image);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect (item, "activate", G_CALLBACK(plugin_unshade_window), win);
    } else {
        item = gtk_image_menu_item_new_with_label ("Shade");
        image = gtk_image_new_from_stock (GTK_STOCK_GOTO_TOP, GTK_ICON_SIZE_MENU);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(item), image);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect (item, "activate", G_CALLBACK(plugin_shade_window), win);
    }

    if (netk_window_is_sticky(win)) {
        item = gtk_image_menu_item_new_with_label ("Unstick");
        image = gtk_image_new_from_stock (GTK_STOCK_REMOVE, GTK_ICON_SIZE_MENU);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(item), image);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect (item, "activate", G_CALLBACK(plugin_unstick_window), win);
    } else {
        item = gtk_image_menu_item_new_with_label ("Stick");
        image = gtk_image_new_from_stock (GTK_STOCK_ADD, GTK_ICON_SIZE_MENU);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(item), image);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect (item, "activate", G_CALLBACK(plugin_stick_window), win);
    }
    
    item = gtk_separator_menu_item_new ();
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    item = gtk_image_menu_item_new_with_label ("Close");
    image = gtk_image_new_from_stock (GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(item), image);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect (item, "activate", G_CALLBACK(plugin_close_window), win);
    
    return (menu);
}

static void
popup_action_menu (GtkWidget *widget, gpointer data)
{
    NetkWindow *win = data;
    static GtkWidget *menu = NULL;

    if (menu) {
        gtk_widget_destroy(menu);
    }

    menu = plugin_create_action_menu (win);

    gtk_widget_show_all (menu);
    gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 0, GDK_CURRENT_TIME);    
}

static void
menu_item_clicked (GtkWidget *item, GdkEventButton *ev, gpointer data)
{
    NetkWindow *win = data;

    if (ev->button == 1) {
        plugin_activate_window (NULL, win);
    } else if (ev->button == 3) {
        popup_action_menu (item, win);
    }

    gtk_menu_popdown (GTK_MENU(gtk_widget_get_parent(item)));
}

// the following function is stolen from xfdesktop
static void
set_num_screens (gpointer num)
{
    static Atom xa_NET_NUMBER_OF_DESKTOPS = 0;
    XClientMessageEvent sev;
    int n;

    if (!xa_NET_NUMBER_OF_DESKTOPS)
    {
	xa_NET_NUMBER_OF_DESKTOPS =
	    XInternAtom (GDK_DISPLAY (), "_NET_NUMBER_OF_DESKTOPS", False);
    }

    n = GPOINTER_TO_INT (num);

    sev.type = ClientMessage;
    sev.display = GDK_DISPLAY ();
    sev.format = 32;
    sev.window = GDK_ROOT_WINDOW ();
    sev.message_type = xa_NET_NUMBER_OF_DESKTOPS;
    sev.data.l[0] = n;

    gdk_error_trap_push ();

    XSendEvent (GDK_DISPLAY (), GDK_ROOT_WINDOW (), False,
		SubstructureNotifyMask | SubstructureRedirectMask,
		(XEvent *) & sev);

    gdk_flush ();
    gdk_error_trap_pop ();
}

// the following two functions are based on xfdesktop code
static GtkWidget *
create_menu_item (NetkWindow *win, GList **unref_needed)
{	
    const char *wname = NULL;
    GtkWidget *item;
    GString *label;
    GdkPixbuf *icon = NULL, *tmp;

    if (netk_window_is_skip_pager (win) || netk_window_is_skip_tasklist (win))
	return NULL;

    wname = netk_window_get_name (win);
    label = g_string_new (wname);
    if (label->len >= 20) {
        g_string_truncate (label, 20);
        g_string_append (label, " ...");
    }

    if (netk_window_is_minimized (win)) {
        g_string_prepend (label, "[");
        g_string_append (label, "]");
    }
    
    tmp = netk_window_get_icon(win);
    if (tmp) {
        gint w, h;
        w = gdk_pixbuf_get_width(tmp);
        h = gdk_pixbuf_get_height(tmp);
        if (w != 22 || h != 22) {
            icon = gdk_pixbuf_scale_simple(tmp, 24, 24, GDK_INTERP_BILINEAR);
	    *unref_needed = g_list_prepend(*unref_needed, icon);
        } else {
            icon = tmp;
	}
    }
	
    if (icon) {
        GtkWidget *img = gtk_image_new_from_pixbuf(icon);
        gtk_widget_show(img);
        item = gtk_image_menu_item_new_with_label(label->str);
        gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), img);
    } else {
        item = gtk_menu_item_new_with_label (label->str);
    }
    
    gtk_tooltips_set_tip (tooltips, item, wname, NULL);
    
    g_string_free (label, TRUE);
    return item;
}

static void
plugin_windowlist_clicked (GtkWidget *w, gpointer data)
{
    gui *plugin = data;
    static GtkWidget *menu = NULL;
    GtkWidget *item, *label;
    NetkWindow *win;
    NetkWorkspace *ws, *aws, *winws;
    NetkScreen *s;
    int wscount, i;
    GList *windows, *li, *l, *unref_needed=NULL;
    GtkStyle *style;

    if (menu) {
        gtk_widget_destroy(menu);
    }

    s = netk_screen_get_default();
    windows = netk_screen_get_windows_stacked(s);
    aws = netk_screen_get_active_workspace (s);

    menu = gtk_menu_new ();

    style = gtk_widget_get_style (menu);

    item = gtk_menu_item_new_with_label N_("Window list");
    gtk_widget_set_sensitive (item, FALSE);
    gtk_menu_shell_append (GTK_MENU_SHELL(menu), item);
    
    item = gtk_separator_menu_item_new ();
    gtk_menu_shell_append (GTK_MENU_SHELL(menu), item);

    if (plugin->includeAll) {
        wscount = netk_screen_get_workspace_count(s);
    } else {
        wscount = 1;
    }
    
    for (i = 0; i<wscount; i++) {
        char *ws_name;
	const char *realname;

        if (plugin->includeAll) {
            ws = netk_screen_get_workspace(s, i);
	} else {
	    ws = netk_screen_get_active_workspace (s);
	}
	
	realname = netk_workspace_get_name (ws);

	if (realname) {
	    ws_name = g_strdup_printf ("<i>%s</i>", realname);
	} else {
	    ws_name = g_strdup_printf ("<i>%d</i>", i + 1);
	}

	item = gtk_menu_item_new_with_label (ws_name);
	g_signal_connect (item, "activate", G_CALLBACK(plugin_activate_workspace), ws);
	g_free (ws_name);
	
	label = gtk_bin_get_child (GTK_BIN (item));
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
        
	gtk_menu_shell_append (GTK_MENU_SHELL(menu), item);

	if (ws == aws) {
	    gtk_widget_set_sensitive (item, FALSE);
	}

	item = gtk_separator_menu_item_new ();
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    	for (li = windows; li; li = li->next ) {
    	    win = li->data;
	    winws = netk_window_get_workspace (win);
	    if (ws != winws && !(netk_window_is_sticky(win) && ws == aws)) {
	        continue;
	    }

	    item = create_menu_item (win, &unref_needed);

	    if (!item)
	        continue;

	    if (ws != aws) {
		gtk_widget_modify_fg (gtk_bin_get_child (GTK_BIN (item)), GTK_STATE_NORMAL, &(style->fg[GTK_STATE_INSENSITIVE]));
	    }

	    g_signal_connect (item, "button-release-event", G_CALLBACK(menu_item_clicked), win);
	
	    gtk_menu_shell_append (GTK_MENU_SHELL(menu), item);
        }

	item = gtk_separator_menu_item_new ();
	gtk_menu_shell_append (GTK_MENU_SHELL(menu), item);
    }
    
    wscount = netk_screen_get_workspace_count (s);
    
    item = gtk_menu_item_new_with_label (_("Add workspace"));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect_swapped (item, "activate", G_CALLBACK (set_num_screens), GINT_TO_POINTER (wscount + 1));

    item = gtk_menu_item_new_with_label (_("Delete workspace"));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect_swapped (item, "activate", G_CALLBACK (set_num_screens), GINT_TO_POINTER (wscount - 1));

    gtk_widget_show_all (menu);
    gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 0, GDK_CURRENT_TIME);    

    if (unref_needed) {
        for(l=unref_needed; l; l=l->next) {
            g_object_unref(G_OBJECT(l->data));
        }
        g_list_free(unref_needed);
        unref_needed = NULL;
    }
}

static void
plugin_active_window_changed (GtkWidget *w, gpointer data)
{
    gui *plugin = data;
    NetkWindow *win;
    GdkPixbuf *pb;
    
    if (win = netk_screen_get_active_window(plugin->screen)) {
        pb = netk_window_get_icon (win);
        if (pb) {
	    xfce_iconbutton_set_pixbuf(XFCE_ICONBUTTON(plugin->windowlist), pb);
	    gtk_tooltips_set_tip (tooltips, plugin->windowlist, netk_window_get_name (win), NULL);
        }
    }
}

static gui *
gui_new ()
{
    gui *plugin;
    GdkPixbuf *pb;

    tooltips = gtk_tooltips_new ();

    plugin = g_new(gui, 1);
    plugin->ebox = gtk_event_box_new();
    
    plugin->includeAll = FALSE;
    plugin->screen=netk_screen_get_default();
    netk_screen_force_update (plugin->screen);

    pb = gtk_widget_render_icon (plugin->ebox, GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_MENU, NULL);
    plugin->windowlist = xfce_iconbutton_new_from_pixbuf(pb);
    g_object_unref (pb);

    gtk_button_set_relief (GTK_BUTTON (plugin->windowlist), GTK_RELIEF_NONE);

    gtk_container_add (GTK_CONTAINER(plugin->ebox), plugin->windowlist);

    gtk_widget_show_all (plugin->ebox);

    plugin->ScreenCallbackId = g_signal_connect(plugin->screen, "active-window-changed", G_CALLBACK(plugin_active_window_changed), plugin);

    g_signal_connect(plugin->windowlist, "clicked", G_CALLBACK(plugin_windowlist_clicked), plugin);
    
    return(plugin);
}

static gboolean
create_plugin_control (Control *ctrl)
{
    gui *plugin = gui_new();

    gtk_container_add (GTK_CONTAINER(ctrl->base), plugin->ebox);

    ctrl->data = (gpointer)plugin;
    ctrl->with_popup = FALSE;

    return(TRUE);
}

static void
plugin_free (Control *ctrl)
{
    gui *plugin;
    g_return_if_fail (ctrl != NULL);
    g_return_if_fail (ctrl->data != NULL);

    plugin = ctrl->data;

    g_signal_handler_disconnect (plugin->screen, plugin->ScreenCallbackId);
    g_free(plugin);
}

static void
plugin_attach_callback (Control *ctrl, const gchar *signal, GCallback cb, gpointer data)
{
    gui *plugin = ctrl->data;
    g_signal_connect (plugin->ebox, signal, cb, data);
    g_signal_connect (plugin->windowlist, signal, cb, data);
}

static void
plugin_read_config (Control *ctrl, xmlNodePtr node)
{
    xmlChar *value;
    gui *plugin = ctrl->data;

    for (node = node->children; node; node = node->next) {
        if (xmlStrEqual(node->name, (const xmlChar *)"Windowlist")) {
            if ((value = xmlGetProp(node, (const xmlChar *)"includeAll"))) {
                plugin->includeAll = atoi(value);
                g_free(value);
	    }
	    break;
	}
    }
}

static void
plugin_write_config (Control *ctrl, xmlNodePtr parent)
{
    gui *plugin = ctrl->data;
    xmlNodePtr root;
    char value[20];

    root = xmlNewTextChild(parent, NULL, "Windowlist", NULL);

    g_snprintf(value, 10, "%d", plugin->includeAll);
    xmlSetProp(root, "includeAll", value);
}    

static void
plugin_cb1_changed (GtkToggleButton *cb, gui *plugin)
{
    gboolean includeAll;

    includeAll = gtk_toggle_button_get_active (cb);

    plugin->includeAll = includeAll;
}

static void
plugin_create_options (Control *ctrl, GtkContainer *con, GtkWidget *done)
{
    gui *plugin = ctrl->data;
    GtkWidget *cb1, *frame;
    frame = gtk_frame_new (NULL);

    cb1 = gtk_check_button_new_with_label ("Include all Workspaces");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb1), plugin->includeAll);
    g_signal_connect (cb1, "toggled", G_CALLBACK (plugin_cb1_changed), plugin);
    
    gtk_container_add (GTK_CONTAINER(frame), cb1);
    gtk_container_add (GTK_CONTAINER(con), frame);
    gtk_widget_show_all (frame);
}

// }}}

// initialization {{{
G_MODULE_EXPORT void
xfce_control_class_init(ControlClass *cc)
{
    xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
    /* these are required */
    cc->name		= "windowlist";
    cc->caption		= _("Windowlist");

    cc->create_control	= (CreateControlFunc)create_plugin_control;

    cc->free		= plugin_free;
    cc->attach_callback	= plugin_attach_callback;

    /* options; don't define if you don't have any ;) */
    cc->read_config	= plugin_read_config;
    cc->write_config	= plugin_write_config;

    cc->create_options          = plugin_create_options;
}

/* required! defined in panel/plugins.h */
XFCE_PLUGIN_CHECK_INIT

// }}}

// vim600: set foldmethod=marker: foldmarker={{{,}}}
