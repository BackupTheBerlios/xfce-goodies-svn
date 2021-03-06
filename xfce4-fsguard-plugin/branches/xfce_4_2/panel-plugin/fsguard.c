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
#include <sys/types.h>
#include <sys/stat.h>
#if defined(__linux__)
#include <sys/vfs.h>
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#include <sys/param.h>
#include <sys/mount.h>
#endif
#include <panel/plugins.h>
#include <panel/xfce.h>
#include <panel/xfce_support.h>
#include "icons.h"

#define HORIZONTAL 0
#define VERTICAL 1

#define TINY 0
#define SMALL 1
#define MEDIUM 2
#define LARGE 3

#define ICONSIZETINY 24 
#define ICONSIZESMALL 30
#define ICONSIZEMEDIUM 45
#define ICONSIZELARGE 60


// }}}

// struct {{{

typedef struct
{
    GtkWidget	    *fs;
    GtkWidget       *hbox;
    GtkWidget       *vbox;
    GtkWidget	    *ebox;
    GtkWidget       *lab;
    gboolean        seen;
    gint            size;
    gint            timeout;
    gint            yellow;
    gint            red;
    gint            orientation;
    gchar           *label;
    gchar           *mnt;
    gchar 	    *filemanager;
} gui;

static GtkTooltips *tooltips = NULL;

// }}}

// all functions {{{

static void
plugin_recreate_gui (gpointer data)
{
    gui *plugin = data;
    if (plugin->label != NULL && (strlen (plugin->label) > 0)) {
        if (plugin->lab == NULL) {
            plugin->lab = gtk_label_new (plugin->label);
	    gtk_widget_show (plugin->lab);
            gtk_box_pack_start (GTK_BOX(plugin->hbox), plugin->lab, FALSE, FALSE, 1);
	    gtk_box_reorder_child (GTK_BOX(plugin->hbox), plugin->lab, 0);
	} else {
	    if (gtk_label_get_text (GTK_LABEL(plugin->lab)) != plugin->label) {
	        gtk_label_set_text (GTK_LABEL(plugin->lab), plugin->label);
	    }
	}
    } else {
        if (GTK_IS_WIDGET (plugin->lab)) {
            gtk_widget_destroy (plugin->lab);
	    plugin->lab = NULL;
	}
    }
        if (plugin->orientation == VERTICAL) {
            gtk_widget_reparent (plugin->fs, plugin->vbox);
            gtk_widget_reparent (plugin->lab, plugin->vbox);
        } else {
            gtk_widget_reparent (plugin->fs, plugin->hbox);
            gtk_widget_reparent (plugin->lab, plugin->hbox);
	}
}

static void
plugin_open_mnt (GtkWidget *widget, gpointer user_data)
{
    gui *plugin = user_data;
    GString *cmd;
    if (strlen(plugin->filemanager) == 0) {
        return;
    }
    cmd = g_string_new (plugin->filemanager);
    if (plugin->mnt != NULL && (strcmp(plugin->mnt, ""))) {
        g_string_append (cmd, " ");
        g_string_append (cmd, plugin->mnt);
    }
    exec_cmd (cmd->str, FALSE, FALSE);
    g_string_free (cmd, TRUE);
}

static gboolean
plugin_check_fs (gpointer data)
{
    GdkPixbuf *pb, *tmp;
    GString *tool;
    float size = 0;
    float freeblocks = 0;
    long blocksize;
    int err;
    gchar msg[100];
    static struct statfs fsd;
    gui *plugin = data;

int statfs (const char *path, struct statfs *buf, int len, int fstyp);
    err = statfs (plugin->mnt, &fsd);
    xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
    
    if (err != -1) {
        blocksize = fsd.f_bsize;
        freeblocks = fsd.f_bavail;
        size = (freeblocks * blocksize) / 1048576;
        if (size <= plugin->red) {
            tmp = gdk_pixbuf_new_from_inline (sizeof(icon_red), icon_red, FALSE, NULL);
	    if (!plugin->seen) {
                if (plugin->label != NULL && (strcmp(plugin->label,"")) && (strcmp(plugin->mnt, plugin->label))) {
		    if (size > 1024) {
                        xfce_warn (_("Only %.2f GB space left on %s (%s)!"), size/1024, plugin->mnt, plugin->label);
		    } else {
                        xfce_warn (_("Only %.2f MB space left on %s (%s)!"), size, plugin->mnt, plugin->label);
		    }
                } else {
		    if (size > 1024) {
                        xfce_warn (_("Only %.2f GB space left on %s!"), size/1024, plugin->mnt);
		    } else {
                        xfce_warn (_("Only %.2f MB space left on %s!"), size, plugin->mnt);
		    }
		}
		plugin->seen = TRUE;
	    }
        } else if (size >= plugin->red && size <= plugin->yellow) {
            tmp = gdk_pixbuf_new_from_inline (sizeof(icon_yellow), icon_yellow, FALSE, NULL);
        } else {
            tmp = gdk_pixbuf_new_from_inline (sizeof(icon_green), icon_green, FALSE, NULL);
        }
        if (plugin->label != NULL && (strcmp(plugin->label,"")) && (strcmp(plugin->mnt, plugin->label))) {
	    if (size > 1024) {
                g_snprintf (msg, sizeof (msg), _("%.2f GB space left on %s (%s)"), size/1024, plugin->mnt, plugin->label);
	    } else {
                g_snprintf (msg, sizeof (msg), _("%.2f MB space left on %s (%s)"), size, plugin->mnt, plugin->label);
	    }
        } else if (plugin->mnt != NULL && (strcmp(plugin->mnt, ""))) {
	    if (size > 1024) {
                g_snprintf (msg, sizeof (msg), _("%.2f GB space left on %s"), size/1024, plugin->mnt);
	    } else {
                g_snprintf (msg, sizeof (msg), _("%.2f MB space left on %s"), size, plugin->mnt);
	    }
        } 
    } else {
        tmp = gdk_pixbuf_new_from_inline (sizeof(icon_unknown), icon_unknown, FALSE, NULL);
        g_snprintf (msg, sizeof (msg), _("could not check mountpoint %s, please check your config"), plugin->mnt);
    }
    
    gtk_tooltips_set_tip (tooltips, plugin->fs, msg, NULL);
       
    pb = gdk_pixbuf_scale_simple (tmp, plugin->size, plugin->size, GDK_INTERP_BILINEAR);
    xfce_iconbutton_set_pixbuf(XFCE_ICONBUTTON(plugin->fs), pb);
    g_object_unref (G_OBJECT(tmp));
    g_object_unref (G_OBJECT(pb));
    return TRUE;
}

static gui *
gui_new ()
{
    gui *plugin;
    tooltips = gtk_tooltips_new ();
    plugin = g_new(gui, 1);
    plugin->ebox = gtk_event_box_new();
    gtk_widget_show (plugin->ebox);
 
    plugin->hbox = gtk_hbox_new(FALSE, 0);
    gtk_widget_show (plugin->hbox);

    plugin->vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (plugin->vbox);
   
    plugin->size = ICONSIZETINY;
    plugin->lab = NULL;
    plugin->label = NULL;
    plugin->mnt = NULL;
    plugin->yellow = 0;
    plugin->red = 0;
    plugin->timeout = 0;
    plugin->filemanager = "xffm";
    plugin->fs = xfce_iconbutton_new ();
    g_signal_connect (G_OBJECT(plugin->fs), "clicked", G_CALLBACK(plugin_open_mnt), plugin);
    gtk_button_set_relief (GTK_BUTTON(plugin->fs), GTK_RELIEF_NONE);
    gtk_container_add (GTK_CONTAINER(plugin->hbox), plugin->vbox);
    gtk_container_add (GTK_CONTAINER(plugin->hbox), plugin->fs);
    gtk_container_add (GTK_CONTAINER(plugin->ebox), plugin->hbox);
    gtk_widget_show_all (plugin->ebox);

    plugin_check_fs (plugin);
    plugin->timeout = g_timeout_add_full (G_PRIORITY_DEFAULT, 8192, (GSourceFunc) plugin_check_fs, plugin, NULL);
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
    if (plugin->timeout != 0) {
        g_source_remove (plugin->timeout);
    }
    g_free(plugin);
}

static void
plugin_attach_callback (Control *ctrl, const gchar *signal, GCallback cb, gpointer data)
{
    gui *plugin = ctrl->data;
    g_signal_connect (plugin->ebox, signal, cb, data);
    g_signal_connect (plugin->fs, signal, cb, data);
}

static void
plugin_set_orientation (Control *ctrl, int orientation)
{
    gui *plugin = ctrl->data;
    plugin->orientation = orientation;
    plugin_recreate_gui (plugin);
}

static void
plugin_set_size (Control *ctrl, int size)
{
    gui *plugin = ctrl->data;
    if (size == TINY) {
        plugin->size = ICONSIZETINY;
    } else if (size == SMALL) {
        plugin->size = ICONSIZESMALL;
    } else if (size == MEDIUM) {
        plugin->size = ICONSIZEMEDIUM;
    } else {
        plugin->size = ICONSIZELARGE;
    }	
    plugin_check_fs (plugin);
    gtk_widget_set_size_request (plugin->fs, plugin->size, plugin->size);
}

static void
plugin_read_config (Control *ctrl, xmlNodePtr node)
{
    xmlChar *value;
    gui *plugin = ctrl->data;

    for (node = node->children; node; node = node->next) {
        if (xmlStrEqual(node->name, (const xmlChar *)"Fsguard")) {
            if ((value = xmlGetProp(node, (const xmlChar *)"yellow"))) {
                plugin->yellow = atoi(value);
                g_free(value);
            }
            if ((value = xmlGetProp(node, (const xmlChar *)"red"))) {
                plugin->red = atoi(value);
                g_free(value);
            }
            if ((value = xmlGetProp(node, (const xmlChar *)"label"))) {
                plugin->label = g_strdup((gchar *)value);
                g_free(value);
            }
            if ((value = xmlGetProp(node, (const xmlChar *)"mnt"))) {
                plugin->mnt = g_strdup((gchar *)value);
                g_free(value);
            }
            if ((value = xmlGetProp(node, (const xmlChar *)"filemanager"))) {
                plugin->filemanager = g_strdup((gchar *)value);
                g_free(value);
            }
            break;
	}
    }
    plugin_recreate_gui (plugin);
    plugin_check_fs (plugin);
    plugin->seen = FALSE;
}

static void
plugin_write_config (Control *ctrl, xmlNodePtr parent)
{
    gui *plugin = ctrl->data;
    xmlNodePtr root;
    char value[20];

    root = xmlNewTextChild(parent, NULL, "Fsguard", NULL);
    g_snprintf(value, 10, "%d", plugin->red);
    xmlSetProp(root, "red", value);

    g_snprintf(value, 10, "%d", plugin->yellow);
    xmlSetProp(root, "yellow", value);

    xmlSetProp(root, "label", plugin->label);

    xmlSetProp(root, "mnt", plugin->mnt);
    
    xmlSetProp(root, "filemanager", plugin->filemanager);
}    

static void
plugin_ent1_changed (GtkWidget *widget, gpointer user_data)
{
    gui *plugin = user_data;
    plugin->label = g_strdup (gtk_entry_get_text (GTK_ENTRY(widget)));
    plugin_recreate_gui (plugin);
}

static void
plugin_ent2_changed (GtkWidget *widget, gpointer user_data)
{
    gui *plugin = user_data;
    plugin->mnt = g_strdup (gtk_entry_get_text (GTK_ENTRY(widget)));
    plugin->seen = FALSE;
    plugin_check_fs (plugin);
}

static void
plugin_ent3_changed (GtkWidget *widget, gpointer user_data)
{
    gui *plugin = user_data;
    plugin->filemanager = g_strdup (gtk_entry_get_text (GTK_ENTRY(widget)));
}

static void
plugin_spin1_changed (GtkWidget *widget, gpointer user_data)
{
    gui *plugin = user_data;
    plugin->red = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(widget));
    plugin->seen = FALSE;
}

static void
plugin_spin2_changed (GtkWidget *widget, gpointer user_data)
{
    gui *plugin = user_data;
    plugin->yellow = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(widget));
}

static void
plugin_create_options (Control *ctrl, GtkContainer *con, GtkWidget *done)
{
    gui *plugin = ctrl->data;
    GtkWidget *hbox, *vbox1, *vbox2, *mnt, *spin1, *spin2;
    GtkWidget *lab1, *lab2, *lab3, *lab4, *lab5, *ent1, *ent2, *ent3;
    gchar *text[] = {
	     N_("label"),
	     N_("mountpoint"),
             N_("high alarm limit (MB)"),
	     N_("high warn limit (MB)"),
	     N_("filemanager"),
    };
    xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
 
    hbox = gtk_hbox_new (FALSE, 2);
    vbox1 = gtk_vbox_new (FALSE, 5);
    vbox2 = gtk_vbox_new (FALSE, 5);

    gtk_box_pack_start (GTK_BOX(hbox), vbox1, TRUE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX(hbox), vbox2, TRUE, FALSE, 1);

    lab1 = gtk_label_new (_(text[0]));
    lab2 = gtk_label_new (_(text[1]));
    lab3 = gtk_label_new (_(text[2]));
    lab4 = gtk_label_new (_(text[3]));
    lab5 = gtk_label_new (_(text[4]));

    ent1 = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY(ent1), 16);
    if (plugin->label != NULL) {
        gtk_entry_set_text (GTK_ENTRY(ent1), plugin->label);
    }
    ent2 = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY(ent2), 32);
    if (plugin->mnt != NULL) {
        gtk_entry_set_text (GTK_ENTRY(ent2), plugin->mnt);
    }
    ent3 = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY(ent3), 16);
    if (plugin->filemanager != NULL) {
        gtk_entry_set_text (GTK_ENTRY(ent3), plugin->filemanager);
    }
 
    spin1 = gtk_spin_button_new_with_range (0, 1000000, 10);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(spin1), plugin->red);
    spin2 = gtk_spin_button_new_with_range (0, 1000000, 10);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(spin2), plugin->yellow);

    g_signal_connect (ent1, "changed", G_CALLBACK(plugin_ent1_changed), plugin);
    g_signal_connect (ent2, "changed", G_CALLBACK(plugin_ent2_changed), plugin);
    g_signal_connect (ent3, "changed", G_CALLBACK(plugin_ent3_changed), plugin);
    g_signal_connect (spin1, "value-changed", G_CALLBACK(plugin_spin1_changed), plugin);
    g_signal_connect (spin2, "value-changed", G_CALLBACK(plugin_spin2_changed), plugin);

    gtk_box_pack_start (GTK_BOX(vbox1), lab1, TRUE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX(vbox1), lab2, TRUE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX(vbox1), lab5, TRUE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX(vbox1), lab3, TRUE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX(vbox1), lab4, TRUE, FALSE, 1);

    gtk_box_pack_start (GTK_BOX(vbox2), ent1, TRUE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX(vbox2), ent2, TRUE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX(vbox2), ent3, TRUE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX(vbox2), spin1, TRUE, FALSE, 1);
    gtk_box_pack_start (GTK_BOX(vbox2), spin2, TRUE, FALSE, 1);

    gtk_container_add (GTK_CONTAINER(con), hbox);
    gtk_widget_show_all (hbox);
}

// }}}

// initialization {{{
G_MODULE_EXPORT void
xfce_control_class_init(ControlClass *cc)
{
    xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
    /* these are required */
    cc->name		= "fsguard";
    cc->caption		= _("Free Space Checker");

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
