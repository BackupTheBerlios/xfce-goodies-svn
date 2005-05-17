/*
 *  xfce4-modem-lights-plugin - a mail notification applet for the xfce4 panel
 *  Copyright (c) 2005 Andreas J. Guelzow <aguelzow@taliesin.ca>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License ONLY.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfcegui4/xfce_scaled_image.h>

#include <panel/plugins.h>
#include <panel/xfce.h>

#include <net/if.h>

#include "modemlights-plugin.h"

#define BORDER 8

#define THEME_OFFLINE_ICON "xfce-utils"
#define THEME_ONLINE_ICON "xfce-utils"
#define THEME_DIALING_ICON "xfce-utils"

typedef enum
{
	modem_disconnected = 1 << 0,
	modem_dialing = 1 << 1,
	modem_connected = 1 << 2
} modemlights_mode_t;

typedef struct
{
	modemlights_mode_t mode;
	gint size;
	guint timer;
	
	GtkWidget *button;
	GtkWidget *image;
	
	GdkPixbuf *pix_online;
	GdkPixbuf *pix_offline;
	GdkPixbuf *pix_dialing;
	
	GtkTooltips *tooltip;
    
	gchar *connection_cmd;
	gchar *disconnection_cmd;
	gchar *device;
	gchar *lockfile;
	gchar *icon_disconnected;
	gchar *icon_connected;
	gchar *icon_dialing;
} XfceModemlightsPlugin;

static gboolean 
interface_is_up (char const *if_name)
{
	struct if_nameindex *interfaces, *saved_interfaces;
	int found = FALSE;

	if ((interfaces = if_nameindex()) == NULL)
		return FALSE;
	saved_interfaces = interfaces;

	while (interfaces->if_index != 0)
	{
		if (strcmp(interfaces->if_name, if_name) == 0)
		{
			found = TRUE;
			break;
		}
		interfaces++;
	}

	if_freenameindex(saved_interfaces);

	return found;
}


static void
modemlights_set_mode (XfceModemlightsPlugin *mwp, modemlights_mode_t mode)
{
	if (mode != mwp->mode) {
		mwp->mode = mode;
		switch (mode) {
		case modem_disconnected:
			if (mwp->pix_offline)
				xfce_scaled_image_set_from_pixbuf
					(XFCE_SCALED_IMAGE(mwp->image),
					 mwp->pix_offline);
			gtk_tooltips_set_tip(mwp->tooltip, mwp->button, 
					     _("No connection"), NULL);
			break;
		case modem_connected:
			if (mwp->pix_online)
				xfce_scaled_image_set_from_pixbuf
					(XFCE_SCALED_IMAGE(mwp->image),
					 mwp->pix_online);
			gtk_tooltips_set_tip(mwp->tooltip, mwp->button, 
					     _("Connection Established"), 
					     NULL);
			break;
		case modem_dialing:
			if (mwp->pix_dialing)
				xfce_scaled_image_set_from_pixbuf
					(XFCE_SCALED_IMAGE(mwp->image),
					 mwp->pix_dialing);
			gtk_tooltips_set_tip(mwp->tooltip, mwp->button, 
					     _("Dialing"), NULL);
			break;
		}
	}
}

static void
modemlights_set_pixmaps (XfceModemlightsPlugin *mwp, gint which)
{
	if (modem_disconnected & which) {
		if(mwp->pix_offline) {
			g_object_unref(G_OBJECT(mwp->pix_offline));
			mwp->pix_offline = NULL;
		}
		if (mwp->icon_disconnected && *mwp->icon_disconnected)
			mwp->pix_offline = gdk_pixbuf_new_from_file_at_size
				(mwp->icon_disconnected, 
				 icon_size[mwp->size], 
				 icon_size[mwp->size], NULL);
		if (!mwp->pix_offline)
			mwp->pix_offline = xfce_themed_icon_load
				(THEME_OFFLINE_ICON, icon_size[mwp->size]);
		if (mwp->mode == modem_disconnected && mwp->pix_offline) 
			xfce_scaled_image_set_from_pixbuf
				(XFCE_SCALED_IMAGE(mwp->image),
				 mwp->pix_offline);
	}
	if (modem_connected & which) {
		if(mwp->pix_online) {
			g_object_unref(G_OBJECT(mwp->pix_online));
			mwp->pix_online = NULL;
		}
		if (mwp->icon_connected && *mwp->icon_connected)
			mwp->pix_online = gdk_pixbuf_new_from_file_at_size
				(mwp->icon_connected, 
				 icon_size[mwp->size], 
				 icon_size[mwp->size], NULL);
		if (!mwp->pix_online)
			mwp->pix_online = xfce_themed_icon_load
				(THEME_ONLINE_ICON, icon_size[mwp->size]);
		if (mwp->mode == modem_connected && mwp->pix_online) 
			xfce_scaled_image_set_from_pixbuf
				(XFCE_SCALED_IMAGE(mwp->image),
				 mwp->pix_online);
	}
	if (modem_dialing & which) {
		if(mwp->pix_dialing) {
			g_object_unref(G_OBJECT(mwp->pix_dialing));
			mwp->pix_dialing = NULL;
		}
		if (mwp->icon_dialing && *mwp->icon_dialing)
			mwp->pix_dialing = gdk_pixbuf_new_from_file_at_size
				(mwp->icon_dialing, 
				 icon_size[mwp->size], 
				 icon_size[mwp->size], NULL);
		if (!mwp->pix_dialing)
			mwp->pix_dialing = xfce_themed_icon_load
				("THEME_DIALING_ICON", icon_size[mwp->size]);
		if (mwp->mode == modem_dialing && mwp->pix_dialing) 
			xfce_scaled_image_set_from_pixbuf
				(XFCE_SCALED_IMAGE(mwp->image),
				 mwp->pix_dialing);
	}
}

static gboolean
modemlights_button_release_cb(GtkWidget *w, GdkEventButton *evt,
			      gpointer user_data)
{
	XfceModemlightsPlugin *mwp = user_data;
	
	if (evt->button != 1)
		return FALSE;

	switch(mwp->mode) {
        case modem_disconnected: 
		if(mwp->connection_cmd && (mwp->connection_cmd)) {
			xfce_exec(mwp->connection_cmd, FALSE, FALSE, NULL);
			modemlights_set_mode (mwp, modem_dialing);
		}
		break;
        case modem_dialing: 
        case modem_connected: 
		if(mwp->disconnection_cmd && (mwp->disconnection_cmd)) {
			xfce_exec(mwp->disconnection_cmd, FALSE, FALSE, NULL);
			modemlights_set_mode (mwp, modem_disconnected);
		}
		break;
	}
    
    return FALSE;
}

static gboolean    
modemlights_timer (gpointer data)
{
	XfceModemlightsPlugin *mwp = data;
	
	if (mwp->lockfile && *mwp->lockfile) {
		if (g_file_test (mwp->lockfile, G_FILE_TEST_EXISTS)) {
			if (mwp->device && *mwp->device 
			    && interface_is_up (mwp->device))
				modemlights_set_mode (mwp, modem_connected);
			else
				modemlights_set_mode (mwp, modem_dialing);
		} else
			modemlights_set_mode (mwp, modem_disconnected);
	}

	return TRUE;
}

static gboolean
modemlights_create(Control *c)
{
	XfceModemlightsPlugin *mwp = g_new0(XfceModemlightsPlugin, 1);
	c->data = mwp;
	
	mwp->mode = modem_disconnected;
	
	mwp->tooltip = gtk_tooltips_new();
	
	mwp->button = gtk_button_new();
	gtk_button_set_relief(GTK_BUTTON(mwp->button), GTK_RELIEF_NONE);
	gtk_widget_show(mwp->button);
	gtk_container_add(GTK_CONTAINER(c->base), mwp->button);
	g_signal_connect(mwp->button, "button-release-event",
			 G_CALLBACK(modemlights_button_release_cb), mwp);
	gtk_tooltips_set_tip(mwp->tooltip, mwp->button, 
			     _("No connection"), NULL);
	
	mwp->image = xfce_scaled_image_new();
	gtk_widget_show(mwp->image);
	gtk_container_add(GTK_CONTAINER(mwp->button), mwp->image);
	
	if(mwp->timer)
		g_source_remove (mwp->timer);
	mwp->timer = g_timeout_add (3000,
				    modemlights_timer,
				    mwp);
 
	return TRUE;
}

static void
modemlights_read_config(Control *c, xmlNodePtr node)
{
    XfceModemlightsPlugin *mwp = c->data;
    xmlChar *value;
    
    value = xmlGetProp(node, (const xmlChar *)"connection_cmd");
    if(value) {
        mwp->connection_cmd = g_strdup(value);
        xmlFree(value);
    }
    value = xmlGetProp(node, (const xmlChar *)"disconnection_cmd");
    if(value) {
        mwp->disconnection_cmd = g_strdup(value);
        xmlFree(value);
    }
    value = xmlGetProp(node, (const xmlChar *)"device");
    if(value) {
        mwp->device = g_strdup(value);
        xmlFree(value);
    }
    value = xmlGetProp(node, (const xmlChar *)"lockfile");
    if(value) {
        mwp->lockfile = g_strdup(value);
        xmlFree(value);
    }
    value = xmlGetProp(node, (const xmlChar *)"icon_disconnected");
    if(value && *value) {
        mwp->icon_disconnected = g_strdup(value);
        xmlFree(value);
	modemlights_set_pixmaps (mwp, modem_disconnected);
    }
    value = xmlGetProp(node, (const xmlChar *)"icon_connected");
    if(value && *value) {
        mwp->icon_connected = g_strdup(value);
        xmlFree(value);
	modemlights_set_pixmaps (mwp, modem_connected);
    }
    value = xmlGetProp(node, (const xmlChar *)"icon_dialing");
    if(value && *value) {
        mwp->icon_dialing = g_strdup(value);
        xmlFree(value);
	modemlights_set_pixmaps (mwp, modem_dialing);
    }
}

static void
modemlights_write_config(Control *c, xmlNodePtr node)
{
    XfceModemlightsPlugin *mwp = c->data;
    
    xmlSetProp(node, (const xmlChar *)"connection_cmd",
            mwp->connection_cmd?mwp->connection_cmd:"");
    xmlSetProp(node, (const xmlChar *)"disconnection_cmd",
            mwp->disconnection_cmd?mwp->disconnection_cmd:"");
    xmlSetProp(node, (const xmlChar *)"device",
            mwp->device?mwp->device:"");
    xmlSetProp(node, (const xmlChar *)"lockfile",
            mwp->lockfile?mwp->lockfile:"");
    xmlSetProp(node, (const xmlChar *)"icon_disconnected",
            mwp->icon_disconnected?mwp->icon_disconnected:"");
    xmlSetProp(node, (const xmlChar *)"icon_connected",
            mwp->icon_connected?mwp->icon_connected:"");
    xmlSetProp(node, (const xmlChar *)"icon_dialing",
            mwp->icon_dialing?mwp->icon_dialing:"");

}

static gboolean
modemlights_lockfile_focusout_cb(GtkWidget *w, GdkEventFocus *evt,
				 gpointer user_data)
{
    XfceModemlightsPlugin *mwp = user_data;
    gchar *filename;
    
    if (mwp->lockfile != NULL)
	    g_free(mwp->lockfile);
    
    filename = gtk_editable_get_chars(GTK_EDITABLE(w), 0, -1);
    mwp->lockfile = (filename != NULL) ? g_strdup(filename) : NULL;
    
    return FALSE;
}

static gboolean
modemlights_connection_cmd_focusout_cb(GtkWidget *w, GdkEventFocus *evt,
				       gpointer user_data)
{
    XfceModemlightsPlugin *mwp = user_data;
    gchar *command;
    
    if (mwp->connection_cmd != NULL)
	    g_free(mwp->connection_cmd);
    
    command = gtk_editable_get_chars(GTK_EDITABLE(w), 0, -1);
    mwp->connection_cmd = (command != NULL) ? g_strdup(command) : NULL;
    
    return FALSE;
}

static gboolean
modemlights_disconnection_cmd_focusout_cb(GtkWidget *w, GdkEventFocus *evt,
					  gpointer user_data)
{
    XfceModemlightsPlugin *mwp = user_data;
    gchar *command;
    
    if (mwp->disconnection_cmd != NULL)
	    g_free(mwp->disconnection_cmd);
    
    command = gtk_editable_get_chars(GTK_EDITABLE(w), 0, -1);
    mwp->disconnection_cmd = (command != NULL) ? g_strdup(command) : NULL;
    
    return FALSE;
}

static gboolean
modemlights_icon_disconnected_focusout_cb(GtkWidget *w, GdkEventFocus *evt,
					  gpointer user_data)
{
    XfceModemlightsPlugin *mwp = user_data;
    gchar *filename;
    
    if (mwp->icon_disconnected != NULL)
	    g_free(mwp->icon_disconnected);
    
    filename = gtk_editable_get_chars(GTK_EDITABLE(w), 0, -1);
    mwp->icon_disconnected = (filename != NULL) ? g_strdup(filename) : NULL;
    modemlights_set_pixmaps (mwp, modem_disconnected);    
    
    return FALSE;
}

static gboolean
modemlights_icon_connected_focusout_cb(GtkWidget *w, GdkEventFocus *evt,
					  gpointer user_data)
{
    XfceModemlightsPlugin *mwp = user_data;
    gchar *filename;
    
    if (mwp->icon_connected != NULL)
	    g_free(mwp->icon_connected);
    
    filename = gtk_editable_get_chars(GTK_EDITABLE(w), 0, -1);
    mwp->icon_connected = (filename != NULL) ? g_strdup(filename) : NULL;
    modemlights_set_pixmaps (mwp, modem_connected);    
    
    return FALSE;
}

static gboolean
modemlights_icon_dialing_focusout_cb(GtkWidget *w, GdkEventFocus *evt,
					  gpointer user_data)
{
    XfceModemlightsPlugin *mwp = user_data;
    gchar *filename;
    
    if (mwp->icon_dialing != NULL)
	    g_free(mwp->icon_dialing);
    
    filename = gtk_editable_get_chars(GTK_EDITABLE(w), 0, -1);
    mwp->icon_dialing = (filename != NULL) ? g_strdup(filename) : NULL;
    modemlights_set_pixmaps (mwp, modem_dialing);    
    return FALSE;
}

static gboolean
modemlights_device_focusout_cb(GtkWidget *w, GdkEventFocus *evt,
			       gpointer user_data)
{
    XfceModemlightsPlugin *mwp = user_data;
    gchar *name;
    
    if (mwp->device != NULL)
	    g_free(mwp->device);
    
    name = gtk_editable_get_chars(GTK_EDITABLE(w), 0, -1);
    mwp->device = (name != NULL) ? g_strdup(name) : NULL;
    
    return FALSE;
}



static void
lf_browse_cb (GtkWidget * b, GtkEntry * entry)
{
	char *file =
		select_file_with_preview (
			_("Select lock file"), 
			gtk_entry_get_text (entry),
			gtk_widget_get_toplevel(GTK_WIDGET(entry)));
                                              
	if (file) {
		gtk_entry_set_text(entry, g_strdup(file));
		g_free (file);
		gtk_widget_grab_focus (GTK_WIDGET(entry));
		gtk_editable_select_region (GTK_EDITABLE (entry), 0, -1);
	}
}

static GtkWidget *
modemlights_create_lockfile_selector (XfceModemlightsPlugin *mwp,
				      char const *prevalue, 
				      GCallback cb, GtkTable *table,
				      int row)
{
	GtkWidget *hbox, *lbl, *entry, *button, *image;

	lbl = gtk_label_new_with_mnemonic(_("Lockfile:"));
	gtk_widget_show(lbl);
	gtk_table_attach (table, lbl, 0, 1, row, row+1, 
			  GTK_FILL, GTK_FILL, BORDER/2, BORDER/2);
	gtk_misc_set_alignment (GTK_MISC(lbl), 0, 0.5);

	hbox = gtk_hbox_new(FALSE, BORDER/2);
	gtk_widget_show(hbox);
	gtk_table_attach (table, hbox, 1, 2, row, row+1, 
			  GTK_FILL, GTK_FILL, BORDER/2, BORDER/2);
	
	entry = gtk_entry_new();
	if(prevalue != NULL)
		gtk_entry_set_text(GTK_ENTRY(entry), prevalue);
	gtk_widget_show(entry);
	gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
	gtk_label_set_mnemonic_widget(GTK_LABEL(lbl), entry);

	button = gtk_button_new();
	image = gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_BUTTON);
	gtk_widget_show(image);
	gtk_container_add (GTK_CONTAINER (button), image);
	gtk_widget_show (button);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	g_signal_connect (button, "clicked",
			  G_CALLBACK (lf_browse_cb), entry);

	g_signal_connect(G_OBJECT(entry), "focus-out-event",
			 cb, mwp);	
	return hbox;
}

static void
ic_browse_cb (GtkWidget * b, GtkEntry * entry)
{
	char *file =
		select_file_with_preview (
			_("Select icon"), 
			gtk_entry_get_text (entry),
			gtk_widget_get_toplevel(GTK_WIDGET(entry)));
                                              
	if (file) {
		gtk_entry_set_text(entry, g_strdup(file));
		g_free (file);
		gtk_widget_grab_focus (GTK_WIDGET(entry));
		gtk_editable_select_region (GTK_EDITABLE (entry), 0, -1);
	}
}

static GtkWidget *
modemlights_create_icon_selector (XfceModemlightsPlugin *mwp, 
				  char const *label, char const *prevalue, 
				  GCallback cb, GtkTable *table,
				  int row)
{
	GtkWidget *hbox, *lbl, *entry, *button, *image;

	lbl = gtk_label_new_with_mnemonic(label);
	gtk_widget_show(lbl);
	gtk_table_attach (table, lbl, 0, 1, row, row+1, 
			  GTK_FILL, GTK_FILL, BORDER/2, BORDER/2);
	gtk_misc_set_alignment (GTK_MISC(lbl), 0, 0.5);

	hbox = gtk_hbox_new(FALSE, BORDER/2);
	gtk_widget_show(hbox);
	gtk_table_attach (table, hbox, 1, 2, row, row+1, 
			  GTK_FILL, GTK_FILL, BORDER/2, BORDER/2);
	
	entry = gtk_entry_new();
	if(prevalue != NULL)
		gtk_entry_set_text(GTK_ENTRY(entry), prevalue);
	gtk_widget_show(entry);
	gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
	gtk_label_set_mnemonic_widget(GTK_LABEL(lbl), entry);

	button = gtk_button_new();
	image = gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_BUTTON);
	gtk_widget_show(image);
	gtk_container_add (GTK_CONTAINER (button), image);
	gtk_widget_show (button);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	g_signal_connect (button, "clicked",
			  G_CALLBACK (ic_browse_cb), entry);

	g_signal_connect(G_OBJECT(entry), "focus-out-event",
			 cb, mwp);	
	return hbox;
}

static GtkWidget *
modemlights_create_entry (XfceModemlightsPlugin *mwp, char const *label, 
			  char const *prevalue, GCallback cb, GtkTable *table,
			  int row)
{
	GtkWidget *hbox, *lbl, *entry;

	
	lbl = gtk_label_new_with_mnemonic(label);
	gtk_widget_show(lbl);
	gtk_table_attach (table, lbl, 0, 1, row, row+1, 
			  GTK_FILL, GTK_FILL, BORDER/2, BORDER/2);
	gtk_misc_set_alignment (GTK_MISC(lbl), 0, 0.5);

	hbox = gtk_hbox_new(FALSE, BORDER/2);
	gtk_widget_show(hbox);
	gtk_table_attach (table, hbox, 1, 2, row, row+1, 
			  GTK_FILL, GTK_FILL, BORDER/2, BORDER/2);

	entry = gtk_entry_new();
	if(prevalue != NULL)
		gtk_entry_set_text(GTK_ENTRY(entry), prevalue);
	gtk_widget_show(entry);
	gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
	gtk_label_set_mnemonic_widget(GTK_LABEL(lbl), entry);

	g_signal_connect(G_OBJECT(entry), "focus-out-event",
			 cb, mwp);	
	return hbox;
}

static void
modemlights_create_options(Control *c, GtkContainer *con, GtkWidget *done)
{
	XfceModemlightsPlugin *mwp = c->data;
	GtkWidget *table;
	int row = 0;
	
	xfce_textdomain(GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");
	
	table = gtk_table_new(7,2,FALSE);
	gtk_widget_show(table);
	gtk_container_add(con, table);
	
/* Connection Command */	
	modemlights_create_entry 
		(mwp, _("_Connection Command:"), 
		 mwp->connection_cmd, 
		 G_CALLBACK(modemlights_connection_cmd_focusout_cb),
		 GTK_TABLE(table), row++);

/* Disconnection Command */	
	modemlights_create_entry 
		(mwp, _("_Disconnection Command:"),
		 mwp->disconnection_cmd, 
		 G_CALLBACK(modemlights_disconnection_cmd_focusout_cb),
		 GTK_TABLE(table), row++);
/* Dev */	
	modemlights_create_entry 
		(mwp, _("De_vice:"), 
		 mwp->device, 
		 G_CALLBACK(modemlights_device_focusout_cb),
		 GTK_TABLE(table), row++);


/* Lock File */	
	modemlights_create_lockfile_selector 
		(mwp, mwp->lockfile, 
		 G_CALLBACK(modemlights_lockfile_focusout_cb),
		 GTK_TABLE(table), row++);


/* Disconnected Icon */	
	modemlights_create_icon_selector 
		(mwp, _("Icon (Disconnected):"), 
		 mwp->icon_disconnected, 
		 G_CALLBACK(modemlights_icon_disconnected_focusout_cb),
		 GTK_TABLE(table), row++);

	
/* Connecting Icon */	
	modemlights_create_icon_selector 
		(mwp, _("Icon (Connecting):"), 
		 mwp->icon_dialing, 
		 G_CALLBACK(modemlights_icon_dialing_focusout_cb),
		 GTK_TABLE(table), row++);
	
/* Connected Icon */	
	modemlights_create_icon_selector 
		(mwp, _("Icon (Connected):"), 
		 mwp->icon_connected, 
		 G_CALLBACK(modemlights_icon_connected_focusout_cb),
		 GTK_TABLE(table), row++);
	
}

static void
modemlights_free(Control *c)
{
	XfceModemlightsPlugin *mwp = c->data;
    
	if(mwp->timer) {
		g_source_remove (mwp->timer);
		mwp->timer = 0;
	}

	if(mwp->pix_online)
		g_object_unref(G_OBJECT(mwp->pix_online));
	if(mwp->pix_offline)
		g_object_unref(G_OBJECT(mwp->pix_offline));
	if(mwp->pix_dialing)
		g_object_unref(G_OBJECT(mwp->pix_dialing));

	if (mwp->connection_cmd)
		g_free(mwp->connection_cmd);
	if (mwp->disconnection_cmd)
		g_free(mwp->disconnection_cmd);
	if (mwp->device)
		g_free(mwp->device);
	if (mwp->lockfile)
		g_free(mwp->lockfile);
	if (mwp->icon_disconnected)
		g_free(mwp->icon_disconnected);
	if (mwp->icon_connected)
		g_free(mwp->icon_connected);
	if (mwp->icon_dialing)
		g_free(mwp->icon_dialing);
	
	gtk_object_sink(GTK_OBJECT(mwp->tooltip));
	
	g_free(mwp);
	c->data = NULL;
}

static void
modemlights_attach_callback(Control *c, const gchar *signal, 
			    GCallback callback,
			    gpointer data)
{
	XfceModemlightsPlugin *mwp = c->data;
	
	g_signal_connect(G_OBJECT(mwp->button), signal, callback, data);
}


static void
modemlights_set_size (Control *c, gint size)
{
	XfceModemlightsPlugin *mwp = c->data;
	gint wsize = icon_size[size] + border_width;

	mwp->size = size;

	modemlights_set_pixmaps 
		(mwp, modem_disconnected + modem_dialing + modem_connected);
	
	gtk_widget_set_size_request(c->base, wsize, wsize);
}

G_MODULE_EXPORT void
xfce_control_class_init(ControlClass *cc)
{
	cc->name = "xfce-modem-lights";
	
	xfce_textdomain(GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");
	cc->caption = _("Modem Lights");
	
	cc->create_control = modemlights_create;
	cc->read_config = modemlights_read_config;
	cc->write_config = modemlights_write_config;
	cc->create_options = modemlights_create_options;
	cc->free = modemlights_free;
	cc->attach_callback = modemlights_attach_callback;
	cc->set_size = modemlights_set_size;
	cc->set_orientation = NULL;
}


XFCE_PLUGIN_CHECK_INIT
