/*
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
#include <bonobo/bonobo-widget.h>
#include <bonobo/bonobo-ui-component.h>
#include <bonobo/bonobo-control-frame.h>
#include <bonobo/bonobo-ui-util.h>
#include <bonobo/bonobo-ui-main.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfcegui4/libxfcegui4.h>
#include "xfapplet.h"

/* Relevant menu items order in the xfce panel popup menu. */
#define MOVE_MENU_ITEM_ORDER            4
#define REMOVE_MENU_ITEM_ORDER          6
#define ADD_MENU_ITEM_ORDER             8
#define CUSTOMIZE_MENU_ITEM_ORDER       9

/* Gnome panel sizes. */
#define GNOME_PANEL_SIZE_XX_SMALL       12
#define GNOME_PANEL_SIZE_X_SMALL        24
#define GNOME_PANEL_SIZE_SMALL          36
#define GNOME_PANEL_SIZE_MEDIUM         48
#define GNOME_PANEL_SIZE_LARGE          64
#define GNOME_PANEL_SIZE_X_LARGE        80
#define GNOME_PANEL_SIZE_XX_LARGE       128

static void
xfapplet_about_dialog (XfcePanelPlugin *plugin, gpointer data)
{
	XfceAboutInfo  *info;
	GtkWidget      *dialog;
	guint           i;
	static const XfAppletTranslators translators[] = {
		{"Daichi Kawahata", "daichi@xfce.org", "ja",},
		{"Adriano Winter Bess", "awbess@gmail.com", "pt_BR",},
	};

	info = xfce_about_info_new ("XfApplet", VERSION " (r" REVISION ")",
				    _("Display Gnome applets on the Xfce4 Panel"),
				    XFCE_COPYRIGHT_TEXT ("2006", "Adriano Winter Bess"), XFCE_LICENSE_GPL);
	xfce_about_info_set_homepage (info, "http://xfce-goodies.berlios.de");
	xfce_about_info_add_credit (info, "Adriano Winter Bess", "awbess@gmail.com", _("Author/Maintainer"));

	for (i = 0; translators[i].name != NULL; i++) {
		gchar *s;
		s = g_strdup_printf (_("Translator (%s)"), translators[i].language);
		xfce_about_info_add_credit (info, translators[i].name, translators[i].email, s);
		g_free (s);
	}

	dialog = xfce_about_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
					info, NULL);
	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
	gtk_dialog_run (GTK_DIALOG (dialog));

	gtk_widget_destroy (dialog);
	xfce_about_info_free (info);
}

static void
xfapplet_about_item_activated (BonoboUIComponent *uic, gpointer data, const char *cname)
{
	XfAppletPlugin *xap = (XfAppletPlugin*) data;

	xfapplet_about_dialog (xap->plugin, xap);
}

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
	xfce_textdomain("xfce4-panel", LIBXFCE4PANEL_LOCALE_DIR, "UTF-8");
        bonobo_ui_util_set_ui (uic, PKGDATADIR "/ui", "XFCE_Panel_Popup.xml",
			       "xfce4-xfapplet-plugin", CORBA_OBJECT_NIL);
	xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

	xfapplet_setup_menu_items (xap->plugin, uic);
	bonobo_ui_component_add_verb (uic, "About", xfapplet_about_item_activated, xap);

	gtk_widget_show (GTK_WIDGET(bw));
	gtk_container_add(GTK_CONTAINER(xap->plugin), GTK_WIDGET(bw));
}

static void
xfapplet_free(XfcePanelPlugin *plugin, XfAppletPlugin *xap)
{
	g_free (xap->iid);
	g_free (xap->gconf_key);
	g_free (xap);
}

static const gchar*
xfapplet_get_size_string (XfcePanelPlugin *plugin)
{
	gint size;
	const gchar *retval = NULL;

	size = xfce_panel_plugin_get_size (plugin);

        if (size <= GNOME_PANEL_SIZE_XX_SMALL)
                retval = "xx-small";
        else if (size <= GNOME_PANEL_SIZE_X_SMALL)
                retval = "x-small";
        else if (size <= GNOME_PANEL_SIZE_SMALL)
                retval = "small";
        else if (size <= GNOME_PANEL_SIZE_MEDIUM)
                retval = "medium";
        else if (size <= GNOME_PANEL_SIZE_LARGE)
                retval = "large";
        else if (size <= GNOME_PANEL_SIZE_X_LARGE)
                retval = "x-large";
        else
                retval = "xx-large";

        return retval;
}

static const gchar*
xfapplet_get_orient_string (XfcePanelPlugin *plugin)
{
        XfceScreenPosition  pos;
        const gchar        *retval = NULL;

        pos = xfce_panel_plugin_get_screen_position (plugin);

        switch (pos) {
        case XFCE_SCREEN_POSITION_NW_H:   /* top */
	case XFCE_SCREEN_POSITION_N:
	case XFCE_SCREEN_POSITION_NE_H:
                retval = "down";
                break;
        case XFCE_SCREEN_POSITION_SW_H:   /* bottom */
	case XFCE_SCREEN_POSITION_S:
	case XFCE_SCREEN_POSITION_SE_H:
                retval = "up";
                break;
        case XFCE_SCREEN_POSITION_NW_V:   /* left */
	case XFCE_SCREEN_POSITION_W:
	case XFCE_SCREEN_POSITION_SW_V:
                retval = "right";
                break;
        case XFCE_SCREEN_POSITION_NE_V:   /* right */
	case XFCE_SCREEN_POSITION_E:
	case XFCE_SCREEN_POSITION_SE_V:
                retval = "left";
                break;
	case XFCE_SCREEN_POSITION_FLOATING_H:
		retval = "down";
		break;
	case XFCE_SCREEN_POSITION_FLOATING_V:
		retval = "right";
		break;
        default:
                g_assert_not_reached ();
                break;
        }

        return retval;
}

static gchar*
xfapplet_construct_moniker (XfAppletPlugin *xap)
{
	gchar *moniker;

	/* We (still) do not support "background" and "locked_down" options. */
	moniker = g_strdup_printf ("%s!prefs_key=/apps/xfce4-panel/xfapplets/%s/prefs;"
				   "orient=%s;size=%s",
				   xap->iid, xap->gconf_key,
				   xfapplet_get_orient_string (xap->plugin),
				   xfapplet_get_size_string (xap->plugin));

	return moniker;
}

void
xfapplet_setup_full (XfAppletPlugin *xap)
{
	GList     *list;
	gchar     *moniker;

	list = gtk_container_get_children (GTK_CONTAINER (xap->plugin));
	if (list && list->data)
		gtk_widget_destroy (GTK_WIDGET (list->data));

	moniker = xfapplet_construct_moniker (xap);
	bonobo_widget_new_control_async (moniker, CORBA_OBJECT_NIL, xfapplet_applet_activated, xap);
	g_free (moniker);
}

static void
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
	xfce_panel_plugin_menu_show_about (xap->plugin);

	g_signal_connect (xap->plugin, "configure-plugin", G_CALLBACK (xfapplet_chooser_dialog), xap);
	g_signal_connect (xap->plugin, "about", G_CALLBACK (xfapplet_about_dialog), xap);
}

static void
xfapplet_save_configuration (XfcePanelPlugin *plugin, gpointer data)
{
	XfceRc         *config;
	gchar          *path;
	XfAppletPlugin *xap = data;

	if (!xap->iid)
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

	/* iid for bonobo control */
	xfce_rc_write_entry (config, "iid", xap->iid);

	/* gconf key for applet preferences */
	xfce_rc_write_entry (config, "gconfkey", xap->gconf_key);

	xfce_rc_close (config);
}

static gboolean
xfapplet_read_configuration (XfAppletPlugin *xap)
{
	XfceRc       *config;
	gchar        *path;
	const gchar  *iid;
	const gchar  *gconf_key;

	path = xfce_panel_plugin_lookup_rc_file (xap->plugin);
	if (!path)
		return FALSE;

	config = xfce_rc_simple_open (path, TRUE);
	g_free (path);

	if (!config)
		return FALSE;

	xfce_rc_set_group (config, "xfapplet");

	/* iid for bonobo control */
	iid = xfce_rc_read_entry (config, "iid", NULL);

	/* gconf key for applet preferences */
	gconf_key = xfce_rc_read_entry (config, "gconfkey", NULL);

	if (!iid || !gconf_key) {
		xfce_rc_close (config);
		return FALSE;
	}
	
	xap->iid = g_strdup (iid);
	xap->gconf_key = g_strdup (gconf_key);

	xfce_rc_close (config);

	return TRUE;
}

static XfAppletPlugin*
xfapplet_new (XfcePanelPlugin *plugin)
{
	XfAppletPlugin *xap;

	xap = g_new0 (XfAppletPlugin, 1);
	xap->plugin = plugin;
	xap->iid = NULL;
	xap->gconf_key = NULL;
	xap->tv = NULL;
	xap->applets = NULL;

	return xap;
}

static void
xfapplet_construct (XfcePanelPlugin *plugin)
{
	int argc = 1;
	char *argv[] = { "xfce4-xfapplet-plugin", };
	XfAppletPlugin *xap;

	xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

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
