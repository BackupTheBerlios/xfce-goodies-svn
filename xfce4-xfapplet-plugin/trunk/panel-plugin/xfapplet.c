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

#include <gtk/gtk.h>
#include <libbonoboui.h>
#include <libxfce4panel/xfce-panel-plugin.h>

typedef struct  {

	XfcePanelPlugin   *plugin;
	GtkWidget         *combo;
	GtkWidget         *eb;
	gulong             configure_signal;
	
} XfAppletPlugin;

static void
xfapplet_load_applet (XfAppletPlugin *xap)
{
	GtkWidget *bw, *eb;
	gchar *applet;

	g_signal_handler_disconnect (xap->plugin, xap->configure_signal);
	gtk_widget_destroy (xap->eb);

	applet = gtk_combo_box_get_active_text (GTK_COMBO_BOX(xap->combo));
	bw = bonobo_widget_new_control (applet, CORBA_OBJECT_NIL);
	gtk_widget_show(bw);
	
	eb = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(eb), bw);
	gtk_event_box_set_above_child(GTK_EVENT_BOX(eb), TRUE);
	gtk_widget_show(eb);
	
	gtk_container_add (GTK_CONTAINER (xap->plugin), eb);
	xfce_panel_plugin_add_action_widget (xap->plugin, eb);

	g_free (applet);
}

static void
xfapplet_free(XfcePanelPlugin *plugin, XfAppletPlugin *xap)
{
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


static void
xfapplet_options_response (GtkWidget *dialog, int response, XfAppletPlugin *xap)
{
	xfapplet_load_applet (xap);
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

static XfAppletPlugin*
xfapplet_new (XfcePanelPlugin *plugin)
{
	XfAppletPlugin *xap;
	GtkWidget *ask, *eb;

	xap = g_new0 (XfAppletPlugin, 1);
	xap->plugin = plugin;

	ask = gtk_label_new (" ?? ");
	gtk_widget_show (ask);

	eb = gtk_event_box_new ();
	xap->eb = eb;
	gtk_container_add (GTK_CONTAINER (eb), ask);
	gtk_widget_show (eb);
	
	gtk_container_add (GTK_CONTAINER(plugin), eb);
	xfce_panel_plugin_add_action_widget (plugin, eb);

	return xap;
}

static void
xfapplet_construct (XfcePanelPlugin *plugin)
{
	int argc = 1;
	char *argv[] = { "gapplet-plugin", };
	XfAppletPlugin *xap;

	bonobo_ui_init (argv[0], "0.0.1", &argc, argv);

	if (!(xap = xfapplet_new (plugin)))
		exit (1);

	xfce_panel_plugin_menu_show_configure (plugin);

	xap->configure_signal =
		g_signal_connect (plugin, "configure-plugin", G_CALLBACK (xfapplet_options), xap);
	g_signal_connect (plugin, "free-data", G_CALLBACK (xfapplet_free), xap);
}

XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL(xfapplet_construct)

