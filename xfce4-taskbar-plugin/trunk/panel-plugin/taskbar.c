// Copyright {{{ 

/*
 * Copyright (c) 2003 Andre Lerche <a.lerche@gmx.net>
 * Copyright (c) 2003 Benedikt Meurer <benedikt.meurer@unix-ag.uni-siegen.de>
 * Copyright (c) 2004 Brady Eidson <nospam.xfce@bradeeoh.com>
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
#include <stdio.h>
#include <libxfce4util/i18n.h>
#include <panel/plugins.h>
#include <panel/xfce.h>

#include <libxfcegui4/netk-screen.h>

#define HORIZONTAL 0

#define AUTOGROUP 0
#define ALWAYSGROUP 1
#define NEVERGROUP 2

// }}}

// struct {{{

typedef struct
{
    GtkWidget	    *ebox;
    GtkWidget	    *frame;
    GtkWidget	    *taskbar;
    GtkWidget       *spin; // The spin from properties dialog
    GdkScreen       *gscr;
    NetkScreen      *screen;
    int	    	    group;
    gboolean	    includeAll;
    gboolean        expand;
    gulong          callbackID;
    int             expandWidth;
    int		    size;
    int		    width;
    int             orientation;
    gboolean	    showLabel;
} gui;

// }}}

// all functions {{{
static void plugin_recreate_gui (gpointer data);
static gboolean plugin_panel_resize_callback (GtkWidget *widget, GtkAllocation *allocation, gpointer data);
static void plugin_determine_expand_width (gpointer data);

static void
plugin_eval_taskbar_options(gui *plugin)
{
    if (plugin->group == AUTOGROUP) {
        netk_tasklist_set_grouping (NETK_TASKLIST(plugin->taskbar), NETK_TASKLIST_AUTO_GROUP);
    } else if (plugin->group == ALWAYSGROUP) {
        netk_tasklist_set_grouping (NETK_TASKLIST(plugin->taskbar), NETK_TASKLIST_ALWAYS_GROUP);
    } else {
        netk_tasklist_set_grouping (NETK_TASKLIST(plugin->taskbar), NETK_TASKLIST_NEVER_GROUP);
    }

    netk_tasklist_set_include_all_workspaces (NETK_TASKLIST(plugin->taskbar), plugin->includeAll);
    netk_tasklist_set_show_label (NETK_TASKLIST(plugin->taskbar), plugin->showLabel);
}

static gui *
gui_new ()
{
    gui *plugin;

    plugin = g_new(gui, 1);
    plugin->ebox = gtk_event_box_new ();

    plugin->gscr = gdk_screen_get_default ();
    
    plugin->width = 100;
    plugin->group = 0;
    plugin->includeAll = FALSE;
    plugin->expand = FALSE;
    plugin->showLabel = TRUE;
    plugin->screen=netk_screen_get_default ();
    netk_screen_force_update (plugin->screen);

    plugin->frame = gtk_frame_new (NULL);
    plugin->taskbar = netk_tasklist_new (plugin->screen);

    gtk_container_add (GTK_CONTAINER(plugin->frame), plugin->taskbar);
    gtk_container_add (GTK_CONTAINER(plugin->ebox), plugin->frame);

    plugin_eval_taskbar_options (plugin);

    gtk_widget_show_all (plugin->ebox);
    
    plugin->callbackID = g_signal_connect (G_OBJECT(panel.toplevel), "size-allocate", G_CALLBACK(plugin_panel_resize_callback), (gpointer)plugin );
    
    return(plugin);
}

static gboolean
plugin_panel_resize_callback (GtkWidget *widget, GtkAllocation *allocation, gpointer data)
{
    gui *plugin = data;
    int currentExpandWidth = plugin->expandWidth;
  
    if (plugin->expand == TRUE ){
        plugin_determine_expand_width (data);
        if (currentExpandWidth != plugin->expandWidth){
            plugin_recreate_gui (plugin);
        }
    }
    return True;
}

static void
plugin_determine_expand_width (gpointer data)
{
    gui *plugin = data;
    GdkRectangle rect;

    int screen, mainFrame, current, monitor;

    if (!GDK_IS_WINDOW(panel.toplevel->window))
        return;
    
    monitor = gdk_screen_get_monitor_at_window (plugin->gscr, panel.toplevel->window);
    gdk_screen_get_monitor_geometry (plugin->gscr, monitor, &rect);

    if (plugin->orientation == HORIZONTAL){
        screen = rect.width;
        mainFrame = panel.toplevel->allocation.width;
        current = plugin->ebox->allocation.width;
    } else {
        screen = rect.height;
        mainFrame = panel.toplevel->allocation.height;
        current = plugin->ebox->allocation.height;
    }

    plugin->expandWidth = screen - (mainFrame - current);
    
    if (plugin->expandWidth <= 0 ) {
        plugin->expandWidth = 1;
    }
}

static void
plugin_recreate_gui (gpointer data)
{ 
    gui *plugin = data;
    GdkPixbuf *tmp, *pb;
    NetkWindow *win;
    NetkScreen *s;
    NetkWorkspace *ws;
    int width;

    if (!GDK_IS_WINDOW(panel.toplevel->window))
        return;
    
    if (plugin->expand == TRUE) {
        plugin_determine_expand_width (plugin);
        width = plugin->expandWidth;
    } else {
        width = plugin->width;
    }

    if (plugin->orientation == HORIZONTAL) {
        gtk_widget_set_size_request (plugin->frame, width, plugin->size);
    } else {
        gtk_widget_set_size_request (plugin->frame, plugin->size, width);
    }
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
    if (plugin->callbackID != 0) {
        g_signal_handler_disconnect (panel.toplevel, plugin->callbackID);
    }
    g_free(plugin);
}

static void
plugin_attach_callback (Control *ctrl, const gchar *signal, GCallback cb, gpointer data)
{
    gui *plugin = ctrl->data;
    g_signal_connect (plugin->ebox, signal, cb, data);
}

static void
plugin_set_size (Control *ctrl, int size)
{
    gui *plugin = ctrl->data;
    plugin->size = icon_size[size];

    plugin_recreate_gui (plugin);
}

static void
plugin_set_orientation (Control *ctrl, int orientation)
{
    gui *plugin = ctrl->data;
    plugin->orientation = orientation;
  
    plugin_recreate_gui (plugin);
}

static void
plugin_read_config (Control *ctrl, xmlNodePtr node)
{    
    xmlChar *value;
    gui *plugin = ctrl->data;

    for (node = node->children; node; node = node->next) {
        if (xmlStrEqual(node->name, (const xmlChar *)"Taskbar")) {
            if ((value = xmlGetProp(node, (const xmlChar *)"size"))) {
                plugin->width = atoi(value);
                g_free(value);
            }
            if ((value = xmlGetProp(node, (const xmlChar *)"group"))) {
                plugin->group = atoi(value);
                g_free(value);
            }
            if ((value = xmlGetProp(node, (const xmlChar *)"includeAll"))) {
                plugin->includeAll = atoi(value);
                g_free(value);
            }
            if ((value = xmlGetProp(node, (const xmlChar *)"expand"))) {
                plugin->expand = atoi(value);
                g_free(value);
            }
	    
	    if ((value = xmlGetProp(node, (const xmlChar *)"showLabel"))) {
                plugin->showLabel = atoi(value);
                g_free(value);
            }
            break;
        }
    }
    plugin_recreate_gui (plugin);
    plugin_eval_taskbar_options (plugin);
}

static void
plugin_write_config (Control *ctrl, xmlNodePtr parent)
{
    gui *plugin = ctrl->data;
    xmlNodePtr root;
    char value[20];

    root = xmlNewTextChild(parent, NULL, "Taskbar", NULL);
    g_snprintf(value, 10, "%d", plugin->width);
    xmlSetProp(root, "size", value);

    g_snprintf(value, 10, "%d", plugin->group);
    xmlSetProp(root, "group", value);

    g_snprintf(value, 10, "%d", plugin->includeAll);
    xmlSetProp(root, "includeAll", value);

    g_snprintf(value, 10, "%d", plugin->expand);
    xmlSetProp(root, "expand", value);  

    g_snprintf(value, 10, "%d", plugin->showLabel);
    xmlSetProp(root, "showLabel", value);  
}    

static void
plugin_spin_changed (GtkWidget *widget, gpointer data)
{
    gui *plugin = data;
    plugin->width = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(widget));
    if (plugin->expand == FALSE ) {
        plugin_recreate_gui (plugin);
    }
}

static void
plugin_rb1_changed (GtkRadioButton *rb, gpointer data)
{
    gui *plugin = data;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(rb))) {
        plugin->group=NEVERGROUP;
    }
    plugin_eval_taskbar_options(plugin);
}

static void
plugin_rb2_changed (GtkRadioButton *rb, gpointer data)
{
    gui *plugin = data;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(rb))) {
        plugin->group=ALWAYSGROUP;
    }
    plugin_eval_taskbar_options(plugin);
}

static void
plugin_rb3_changed (GtkRadioButton *rb, gpointer data)
{
    gui *plugin = data;
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(rb))) {
        plugin->group=AUTOGROUP;
    }
    plugin_eval_taskbar_options(plugin);
}

static void
plugin_cb1_changed (GtkToggleButton *cb, gui *plugin)
{
    gboolean expand;

    expand = gtk_toggle_button_get_active (cb);

    plugin->expand = expand;
    gtk_widget_set_sensitive (plugin->spin, !expand);
    
    plugin_recreate_gui (plugin);
    plugin_eval_taskbar_options(plugin);
}

static void
plugin_cb2_changed (GtkToggleButton *cb, gui *plugin)
{
    gboolean includeAll;

    includeAll = gtk_toggle_button_get_active (cb);

    plugin->includeAll = includeAll;
    
    plugin_eval_taskbar_options(plugin);
}

static void
plugin_cb3_changed (GtkToggleButton *cb, gui *plugin)
{
    gboolean showLabel;

    showLabel = gtk_toggle_button_get_active (cb);

    plugin->showLabel = showLabel;
    
    plugin_eval_taskbar_options(plugin);
}

static void
plugin_create_options (Control *ctrl, GtkContainer *con, GtkWidget *done)
{
    gui *plugin = ctrl->data;
    GtkWidget *hbox, *size, *rb1, *rb2, *rb3, *cb1, *cb2, *cb3, *vbox, *frame, *rvbox;
    hbox = gtk_hbox_new (FALSE, 2);
    vbox = gtk_vbox_new (FALSE, 4);
    rvbox = gtk_vbox_new (FALSE, 2);

    size = gtk_label_new ("Size");

    plugin->spin = gtk_spin_button_new_with_range (1,1000,1.0);

    gtk_spin_button_set_value (GTK_SPIN_BUTTON(plugin->spin), plugin->width);
    gtk_widget_set_sensitive (plugin->spin, !plugin->expand);
    
    g_signal_connect (plugin->spin, "value-changed", G_CALLBACK (plugin_spin_changed), plugin);

    frame = gtk_frame_new ("group options");
    rb1 = gtk_radio_button_new_with_label (NULL, "never");
    rb2 = gtk_radio_button_new_with_label (gtk_radio_button_get_group(GTK_RADIO_BUTTON(rb1)), "always");
    rb3 = gtk_radio_button_new_with_label (gtk_radio_button_get_group(GTK_RADIO_BUTTON(rb1)), "automatic");

    if (plugin->group==AUTOGROUP) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb3), True);
    } else if (plugin->group==ALWAYSGROUP) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb2), True);
    } else {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb1), True);
    }

    g_signal_connect (rb1, "toggled", G_CALLBACK (plugin_rb1_changed), plugin);
    g_signal_connect (rb2, "toggled", G_CALLBACK (plugin_rb2_changed), plugin);
    g_signal_connect (rb3, "toggled", G_CALLBACK (plugin_rb3_changed), plugin);

    gtk_container_add(GTK_CONTAINER(rvbox), rb1);
    gtk_container_add(GTK_CONTAINER(rvbox), rb2);
    gtk_container_add(GTK_CONTAINER(rvbox), rb3);

    cb1 = gtk_check_button_new_with_label ("Expand to fill screen");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb1), plugin->expand);
    g_signal_connect (cb1, "toggled", G_CALLBACK (plugin_cb1_changed), plugin);

    cb2 = gtk_check_button_new_with_label ("Include all Workspaces");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb2), plugin->includeAll);
    g_signal_connect (cb2, "toggled", G_CALLBACK (plugin_cb2_changed), plugin);
    
    cb3 = gtk_check_button_new_with_label ("Show Label");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb3), plugin->showLabel);
    g_signal_connect (cb3, "toggled", G_CALLBACK (plugin_cb3_changed), plugin);
 
    gtk_box_pack_start (GTK_BOX(hbox), size, FALSE, FALSE, 1);
    
    gtk_box_pack_start (GTK_BOX(hbox), plugin->spin, FALSE, FALSE, 1);

    gtk_container_add (GTK_CONTAINER(vbox), hbox);
    gtk_container_add (GTK_CONTAINER(frame), rvbox);
    gtk_container_add (GTK_CONTAINER(vbox), frame);
    gtk_container_add (GTK_CONTAINER(vbox), cb1);
    gtk_container_add (GTK_CONTAINER(vbox), cb2);
    gtk_container_add (GTK_CONTAINER(vbox), cb3);
    gtk_container_add (GTK_CONTAINER(con), vbox);
    gtk_widget_show_all (vbox);
}

// }}}

// initialization {{{
G_MODULE_EXPORT void
xfce_control_class_init(ControlClass *cc)
{
    xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
    /* these are required */
    cc->name		= "taskbar";
    cc->caption		= _("Taskbar");

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
    cc->set_orientation		= plugin_set_orientation;
    cc->create_options          = plugin_create_options;
}

/* required! defined in panel/plugins.h */
XFCE_PLUGIN_CHECK_INIT

// }}}

// vim600: set foldmethod=marker: foldmarker={{{,}}}
