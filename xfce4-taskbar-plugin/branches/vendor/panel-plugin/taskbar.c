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

#define TINY 0
#define SMALL 1
#define MEDIUM 2
#define LARGE 3

#define SIZETINY 24 
#define SIZESMALL 30
#define SIZEMEDIUM 45
#define SIZELARGE 60

// }}}

// struct {{{

typedef struct
{
    GtkWidget	    *ebox;
    GtkWidget	    *frame;
    GtkWidget	    *taskbar;
    NetkScreen      *screen;
    gboolean	    alwaysGroup;
    gboolean	    includeAll;
    int		    size;
    int		    width;
    int             orientation;
} gui;

// }}}

// all functions {{{

static void
plugin_eval_TaskbarOptions(gui *plugin)
{
    netk_tasklist_set_grouping(NETK_TASKLIST(plugin->taskbar), plugin->alwaysGroup ? NETK_TASKLIST_ALWAYS_GROUP : NETK_TASKLIST_AUTO_GROUP);
    netk_tasklist_set_include_all_workspaces(NETK_TASKLIST(plugin->taskbar), plugin->includeAll);
}

static gui *
gui_new ()
{
    gui *plugin;

    plugin = g_new(gui, 1);
    plugin->ebox = gtk_event_box_new();
    
    plugin->width=100;
    plugin->alwaysGroup = TRUE;
    plugin->includeAll = FALSE;
    plugin->screen=netk_screen_get_default();
    netk_screen_force_update (plugin->screen);

    plugin->frame = gtk_frame_new (NULL);
    plugin->taskbar = netk_tasklist_new(plugin->screen);

    plugin_eval_TaskbarOptions(plugin);

    gtk_container_add (GTK_CONTAINER(plugin->frame), plugin->taskbar);
    gtk_container_add (GTK_CONTAINER(plugin->ebox), plugin->frame);

    gtk_widget_show_all (plugin->ebox);
    
    return(plugin);
}

static void
plugin_recreate_gui (gpointer data)
{ 
    gui *plugin = data;
    GdkPixbuf *tmp, *pb;
    NetkWindow *win;
    NetkScreen *s;
    NetkWorkspace *ws;

    if (plugin->orientation==HORIZONTAL) {
    	gtk_widget_set_size_request (plugin->frame, plugin->width, plugin->size);
    } else {
    	gtk_widget_set_size_request (plugin->frame, plugin->size, plugin->width);
    }
    gtk_widget_destroy (plugin->taskbar);
    plugin->taskbar = netk_tasklist_new(plugin->screen);

    gtk_container_add (GTK_CONTAINER(plugin->frame), plugin->taskbar);
    gtk_widget_show (plugin->taskbar);
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
    if (size == TINY) {
        plugin->size = SIZETINY;
    } else if (size == SMALL) {
        plugin->size = SIZESMALL;
    } else if (size == MEDIUM) {
        plugin->size = SIZEMEDIUM;
    } else {
        plugin->size = SIZELARGE;
    }	
    plugin_recreate_gui (plugin);
}

static void
plugin_set_orientation (Control *ctrl, int orientation)
{
    gui *plugin = ctrl->data;
    plugin->orientation = orientation;
    plugin_recreate_gui(plugin);
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
            if ((value = xmlGetProp(node, (const xmlChar *)"alwaysGroup"))) {
                plugin->alwaysGroup = atoi(value);
                g_free(value);
	    }
            if ((value = xmlGetProp(node, (const xmlChar *)"includeAll"))) {
                plugin->includeAll = atoi(value);
                g_free(value);
	    }
	    break;
	}
    }
    plugin_recreate_gui (plugin);
    plugin_eval_TaskbarOptions (plugin);
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

    g_snprintf(value, 10, "%d", plugin->alwaysGroup);
    xmlSetProp(root, "alwaysGroup", value);

    g_snprintf(value, 10, "%d", plugin->includeAll);
    xmlSetProp(root, "includeAll", value);
}    

static void
plugin_spin_changed (GtkWidget *widget, gpointer data)
{
    gui *plugin = data;
    plugin->width = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(widget));
    plugin_recreate_gui(plugin);
}

static void
plugin_cb1_changed (GtkToggleButton *cb, gui *plugin)
{
    gboolean alwaysGroup;

    alwaysGroup = gtk_toggle_button_get_active (cb);

    plugin->alwaysGroup = alwaysGroup;
    
    plugin_eval_TaskbarOptions(plugin);
}

static void
plugin_cb2_changed (GtkToggleButton *cb, gui *plugin)
{
    gboolean includeAll;

    includeAll = gtk_toggle_button_get_active (cb);

    plugin->includeAll = includeAll;
    
    plugin_eval_TaskbarOptions(plugin);
}

static void
plugin_create_options (Control *ctrl, GtkContainer *con, GtkWidget *done)
{
    gui *plugin = ctrl->data;
    GtkWidget *hbox, *size, *spin, *cb1, *cb2, *vbox, *sep;
    hbox = gtk_hbox_new (FALSE, 2);
    vbox = gtk_vbox_new (FALSE, 4);

    size = gtk_label_new ("Size");

    spin = gtk_spin_button_new_with_range (1,1000,1.0);

    gtk_spin_button_set_value (GTK_SPIN_BUTTON(spin), plugin->width);
    
    g_signal_connect (spin, "value-changed", G_CALLBACK (plugin_spin_changed), plugin);

    sep = gtk_hseparator_new ();

    cb1 = gtk_check_button_new_with_label ("Always group tasks");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb1), plugin->alwaysGroup);
    g_signal_connect (cb1, "toggled", G_CALLBACK (plugin_cb1_changed), plugin);

    cb2 = gtk_check_button_new_with_label ("Include all Workspaces");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb2), plugin->includeAll);
    g_signal_connect (cb2, "toggled", G_CALLBACK (plugin_cb2_changed), plugin);
    
    gtk_box_pack_start (GTK_BOX(hbox), size, FALSE, FALSE, 1);
    
    gtk_box_pack_start (GTK_BOX(hbox), spin, FALSE, FALSE, 1);

    gtk_container_add (GTK_CONTAINER(vbox), hbox);
    gtk_container_add (GTK_CONTAINER(vbox), sep);
    gtk_container_add (GTK_CONTAINER(vbox), cb1);
    gtk_container_add (GTK_CONTAINER(vbox), cb2);
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
