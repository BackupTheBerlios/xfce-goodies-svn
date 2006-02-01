/*  $Id: xfapplet.c 2 2006-01-14 14:15:24Z adriano $
 *  xfce4-xfapplet-plugin - a gnome applet displaying plugin for xfce4 panel
 *  Copyright (c) 2006 Adriano Winter Bess <awbess@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License ONLY.
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
#include <libbonoboui.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/xfce-panel-plugin.h>

#define MOVE_MENU_ITEM_ORDER            4
#define REMOVE_MENU_ITEM_ORDER          6
#define ADD_MENU_ITEM_ORDER             8
#define CUSTOMIZE_MENU_ITEM_ORDER       9

typedef struct  {

	XfcePanelPlugin   *plugin;
	gchar             *moniker;
	GtkWidget         *combo;
	
} XfAppletPlugin;

static void
xfapplet_menu_item_activated (BonoboUIComponent *uic, gpointer mi, const char *cname)
{
	gtk_menu_item_activate (GTK_MENU_ITEM(mi));
}

static void
xfapplet_setup_menu_items (XfcePanelPlugin *plugin, BonoboUIComponent *uic)
{
	GtkWidget *menu;
	GList     *list;
	gpointer   data;

	/*
	 * The menu items order inside the menu is defined in
	 * xfce_panel_plugin_create_menu() in the file
	 * xfce-panel-plugin-iface.c from the libxfce4panel sources.
	 */

        menu = g_object_get_data (G_OBJECT(plugin), "xfce-panel-plugin-menu");
	if (!menu)
		return;
        list = gtk_container_get_children (GTK_CONTAINER (menu));

	/* "Customize Panel" menu item */
	data = g_list_nth_data (list, CUSTOMIZE_MENU_ITEM_ORDER);
	if (GTK_IS_MENU_ITEM (data))
		bonobo_ui_component_add_verb (uic, "CustomizePanel",
					      xfapplet_menu_item_activated, data);

	/* "Add New Item" menu item */
	data = g_list_nth_data (list, ADD_MENU_ITEM_ORDER);
	if (GTK_IS_MENU_ITEM (data))
		bonobo_ui_component_add_verb (uic, "Add",
					      xfapplet_menu_item_activated, data);

	/* "Remove" menu item */
	data = g_list_nth_data (list, REMOVE_MENU_ITEM_ORDER);
	if (GTK_IS_MENU_ITEM (data))
		bonobo_ui_component_add_verb (uic, "Remove",
					      xfapplet_menu_item_activated, data);

	/* "Move" menu item */
	data = g_list_nth_data (list, MOVE_MENU_ITEM_ORDER);
	if (GTK_IS_MENU_ITEM (data))
		bonobo_ui_component_add_verb (uic, "Move",
					      xfapplet_menu_item_activated, data);
}

static void
xfapplet_applet_activated (BonoboWidget *bw, CORBA_Environment *ev, gpointer data)
{
	BonoboControlFrame  *frame;
	BonoboUIComponent   *uic;
	XfAppletPlugin      *xap = (XfAppletPlugin*) data;

	frame = bonobo_widget_get_control_frame (bw);
        uic = bonobo_control_frame_get_popup_component (frame, CORBA_OBJECT_NIL);
        bonobo_ui_util_set_ui (uic, PKGDATADIR "/ui", "XFCE_Panel_Popup.xml",
			       "xfce4-xfapplet-plugin", CORBA_OBJECT_NIL);

	xfapplet_setup_menu_items (xap->plugin, uic);
	
	gtk_widget_show (GTK_WIDGET(bw));
	gtk_container_add(GTK_CONTAINER(xap->plugin), GTK_WIDGET(bw));
}

static void
xfapplet_free(XfcePanelPlugin *plugin, XfAppletPlugin *xap)
{
	if (xap->moniker)
		g_free (xap->moniker);
	g_free (xap);
}

static void
xfapplet_create_applet_list(XfAppletPlugin *xap)
{
	Bonobo_ServerInfoList *applets;
        CORBA_Environment env;
        int i;

        CORBA_exception_init (&env);
        applets = bonobo_activation_query ("has (repo_ids, 'IDL:GNOME/Vertigo/PanelAppletShell:1.0')",
					   NULL, &env);
        if (BONOBO_EX (&env)) {
		CORBA_exception_free (&env);
		return;
	}
        CORBA_exception_free (&env);

        for (i = 0; i < applets->_length; i++) {
                Bonobo_ServerInfo *applet;
                applet = &applets->_buffer [i];
		gtk_combo_box_append_text (GTK_COMBO_BOX(xap->combo), applet->iid);
        }

	CORBA_free (applets);
}

static gboolean xfapplet_setup_full (XfAppletPlugin*);

static void
xfapplet_options_response (GtkWidget *dialog, int response, XfAppletPlugin *xap)
{
	xap->moniker = gtk_combo_box_get_active_text (GTK_COMBO_BOX(xap->combo));
	xfapplet_setup_full (xap);
	gtk_widget_destroy (dialog);
	xfce_panel_plugin_unblock_menu (xap->plugin);
}

static void
xfapplet_options(XfcePanelPlugin *plugin, XfAppletPlugin *xap)
{
	GtkWidget *dialog, *label, *combo;

	xfce_panel_plugin_block_menu (plugin);

	dialog = gtk_dialog_new_with_buttons ("Choose an applet",
					      GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
					      GTK_DIALOG_DESTROY_WITH_PARENT,
					      GTK_STOCK_OK,
					      GTK_RESPONSE_OK,
					      NULL);
	g_signal_connect (dialog, "response", G_CALLBACK (xfapplet_options_response), xap);
	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);

	label = gtk_label_new ("Choose an applet:");
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), label, FALSE, FALSE, 2);
	
	combo = gtk_combo_box_new_text ();
	xap->combo = combo;
	xfapplet_create_applet_list (xap);
        gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), combo, FALSE, FALSE, 2);

	gtk_widget_show_all (dialog);
}

static gboolean
xfapplet_setup_full (XfAppletPlugin *xap)
{
	GList     *list;

	list = gtk_container_get_children (GTK_CONTAINER (xap->plugin));
	if (list && list->data)
		gtk_widget_destroy (GTK_WIDGET (list->data));

	bonobo_widget_new_control_async (xap->moniker, CORBA_OBJECT_NIL, xfapplet_applet_activated, xap);
}

static gboolean
xfapplet_setup_empty (XfAppletPlugin *xap)
{
	GtkWidget *ask, *eb;
	
	ask = gtk_label_new (" ?? ");
	gtk_widget_show (ask);

	eb = gtk_event_box_new ();
	gtk_container_add (GTK_CONTAINER (eb), ask);
	gtk_widget_show (eb);
	
	gtk_container_add (GTK_CONTAINER(xap->plugin), eb);
	xfce_panel_plugin_add_action_widget (xap->plugin, eb);
	xfce_panel_plugin_menu_show_configure (xap->plugin);

	g_signal_connect (xap->plugin, "configure-plugin", G_CALLBACK (xfapplet_options), xap);
}

static void
xfapplet_save_configuration (XfcePanelPlugin *plugin, gpointer data)
{
	XfceRc         *config;
	gchar          *path;
	XfAppletPlugin *xap = data;

	if (!xap->moniker)
		return;

	path = xfce_panel_plugin_lookup_rc_file (plugin);
	if (!path)
		path = xfce_panel_plugin_save_location (plugin, TRUE);

	if (!path)
		return;

        config = xfce_rc_simple_open (path, FALSE);
	g_free (path);

	if (!config)
		return;

	xfce_rc_set_group (config, "xfapplet");

	/* Moniker for bonobo control */
	xfce_rc_write_entry (config, "moniker", xap->moniker);

	xfce_rc_close (config);
}

static gboolean
xfapplet_read_configuration (XfAppletPlugin *xap)
{
	XfceRc       *config;
	gchar        *path;
	const gchar  *moniker;

	path = xfce_panel_plugin_lookup_rc_file (xap->plugin);
	if (!path)
		return FALSE;

	config = xfce_rc_simple_open (path, TRUE);
	g_free (path);

	if (!config)
		return FALSE;

	xfce_rc_set_group (config, "xfapplet");

	/* Moniker for bonobo control */
	moniker = xfce_rc_read_entry (config, "moniker", NULL);
	if (!moniker) {
		xfce_rc_close (config);
		return FALSE;
	}
	xap->moniker = g_strdup (moniker);

	xfce_rc_close (config);

	return TRUE;
}

static XfAppletPlugin*
xfapplet_new (XfcePanelPlugin *plugin)
{
	XfAppletPlugin *xap;

	xap = g_new0 (XfAppletPlugin, 1);
	xap->plugin = plugin;
	xap->moniker = NULL;

	return xap;
}

static void
xfapplet_construct (XfcePanelPlugin *plugin)
{
	int argc = 1;
	char *argv[] = { "xfce4-xfapplet-plugin", };
	XfAppletPlugin *xap;

	bonobo_ui_init (argv[0], "0.0.1", &argc, argv);

	if (!(xap = xfapplet_new (plugin)))
		exit (1);

	if (xfapplet_read_configuration (xap))
		xfapplet_setup_full (xap);
	else
		xfapplet_setup_empty (xap);

	g_signal_connect (plugin, "free-data", G_CALLBACK (xfapplet_free), xap);
	g_signal_connect (plugin, "save", G_CALLBACK (xfapplet_save_configuration), xap);
}

XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL(xfapplet_construct)
