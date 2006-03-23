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
#include <bonobo/bonobo-moniker-util.h>
#include <bonobo/bonobo-widget.h>
#include <bonobo/bonobo-ui-component.h>
#include <bonobo/bonobo-control-frame.h>
#include <bonobo/bonobo-ui-util.h>
#include <bonobo/bonobo-exception.h>
#include <bonobo/bonobo-ui-main.h>
#include <gconf/gconf-client.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfcegui4/libxfcegui4.h>
#include "xfapplet.h"

/* Relevant menu items order in the xfce panel popup menu. */
#define PROPERTIES_MENU_ITEM_ORDER	2
#define ABOUT_MENU_ITEM_ORDER		3
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

/* Gnome applet orientations */
#define GNOME_APPLET_ORIENT_UP		0
#define GNOME_APPLET_ORIENT_DOWN	1
#define GNOME_APPLET_ORIENT_LEFT	2
#define GNOME_APPLET_ORIENT_RIGHT	3

enum {
	PANEL_NEAR_TOP,
	PANEL_NEAR_BOTTOM,
	PANEL_NEAR_RIGHT,
	PANEL_NEAR_LEFT
};

typedef struct {
	gchar  *name;
	gchar  *email;
	gchar  *language;
} XfAppletTranslators;

static void
xfapplet_setup_empty (XfAppletPlugin *xap);

static void
xfapplet_cleanup_unused_gconf_keys ()
{
	GConfClient *client;
	gchar	*dir;
	gchar	*in_use_key;
	int	 i = 0;

	client = gconf_client_get_default ();
	
	while (1) {
		dir = g_strdup_printf (XFAPPLET_GCONF_DIR "applet_%d", i++);
		if (gconf_client_dir_exists (client, dir, NULL)) {
			in_use_key = g_strdup_printf ("%s/in_use", dir);
			if (!gconf_client_get_bool (client, in_use_key, NULL))
				gconf_client_recursive_unset (client, dir, GCONF_UNSET_INCLUDING_SCHEMA_NAMES, NULL);
			g_free (in_use_key);
			g_free (dir);
		}
		else
			break;
	}

	g_object_unref (client);

	g_free (dir);
}

static GtkWidget*
xfapplet_get_plugin_child (XfcePanelPlugin *plugin)
{
	GtkWidget	*child = NULL;
	GList		*list;
	
	list = gtk_container_get_children (GTK_CONTAINER (plugin));
	if (list && list->data)
		child = GTK_WIDGET (list->data);

	return child;
}

void
xfapplet_cleanup_current (XfAppletPlugin *xap)
{
	g_free (xap->iid);
	xap->iid = NULL;
	g_free (xap->name);
	xap->name = NULL;
	g_free (xap->gconf_key);
	xap->gconf_key = NULL;
}

static void
xfapplet_reload_response (GtkWidget *dialog, int response, XfAppletPlugin *xap)
{
	if (response == GTK_RESPONSE_YES)
		xfapplet_setup_full (xap);
	else if (response == GTK_RESPONSE_NO) {
		xfapplet_cleanup_current (xap);
		xfapplet_setup_empty (xap);
	}
	else
		g_assert_not_reached ();

	gtk_widget_destroy (dialog);
}

static void
xfapplet_connection_broken (ORBitConnection *conn, XfAppletPlugin *xap)
{
	GtkWidget	*dialog;
	GdkScreen	*screen;

	screen = gtk_widget_get_screen (GTK_WIDGET (xap->plugin));
	dialog = gtk_message_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (xap->plugin))),
					 GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_WARNING,
					 GTK_BUTTONS_NONE,
					 _("'%s' has quit unexpectedly."),
					 xap->name);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						  _("If you don't reload the applet, XfApplet plugin "
						    "will go back to its initial empty state."));
	gtk_dialog_add_buttons (GTK_DIALOG (dialog), _("Don't Reload"), GTK_RESPONSE_NO,
				_("Reload"), GTK_RESPONSE_YES, NULL);
	gtk_window_set_screen (GTK_WINDOW (dialog), screen);
	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
	g_signal_connect (dialog, "response", G_CALLBACK (xfapplet_reload_response), xap);
	gtk_widget_show (dialog);
}

static void
xfapplet_unload_applet (XfAppletPlugin *xap)
{
	if (xap->prop_bag) {
		bonobo_object_release_unref (xap->prop_bag, NULL);
		xap->prop_bag = CORBA_OBJECT_NIL;
	}

	if (xap->uic) {
		bonobo_object_unref (BONOBO_OBJECT (xap->uic));
		xap->uic = NULL;
	}
	
	if (xap->object) {
		ORBit_small_unlisten_for_broken (xap->object, G_CALLBACK (xfapplet_connection_broken));
		CORBA_Object_release (xap->object, NULL);
		xap->object = CORBA_OBJECT_NIL;
	}
}

static int
xfapplet_panel_near (GtkWidget *widget, GtkOrientation orientation)
{
	GdkScreen	*screen;
	gint		 x, y, ret = 0;

	screen = gtk_widget_get_screen (widget);
	gdk_window_get_origin (widget->window, &x, &y);

	if (orientation == GTK_ORIENTATION_HORIZONTAL)
		ret = y > (gdk_screen_get_height (screen) - y) ? PANEL_NEAR_BOTTOM : PANEL_NEAR_TOP;
	else if (orientation == GTK_ORIENTATION_VERTICAL)
		ret = x > (gdk_screen_get_width (screen) - x) ? PANEL_NEAR_RIGHT : PANEL_NEAR_LEFT;
	else
		g_assert_not_reached ();

	return ret;
}

static unsigned short
xfapplet_xfce_screen_position_to_gnome_applet_orientation (XfcePanelPlugin *plugin, XfceScreenPosition position)
{
        switch (position) {
        case XFCE_SCREEN_POSITION_NW_H:   /* top */
	case XFCE_SCREEN_POSITION_N:
	case XFCE_SCREEN_POSITION_NE_H:
                return GNOME_APPLET_ORIENT_DOWN;
        case XFCE_SCREEN_POSITION_SW_H:   /* bottom */
	case XFCE_SCREEN_POSITION_S:
	case XFCE_SCREEN_POSITION_SE_H:
                return GNOME_APPLET_ORIENT_UP;
        case XFCE_SCREEN_POSITION_NW_V:   /* left */
	case XFCE_SCREEN_POSITION_W:
	case XFCE_SCREEN_POSITION_SW_V:
                return GNOME_APPLET_ORIENT_RIGHT;
        case XFCE_SCREEN_POSITION_NE_V:   /* right */
	case XFCE_SCREEN_POSITION_E:
	case XFCE_SCREEN_POSITION_SE_V:
                return GNOME_APPLET_ORIENT_LEFT;
	case XFCE_SCREEN_POSITION_FLOATING_H:
		switch (xfapplet_panel_near (GTK_WIDGET (plugin), GTK_ORIENTATION_HORIZONTAL)) {
		case PANEL_NEAR_TOP:
			return GNOME_APPLET_ORIENT_DOWN;
		case PANEL_NEAR_BOTTOM:
			return GNOME_APPLET_ORIENT_UP;
		default:
			g_assert_not_reached ();
			break;
		}
		break;
	case XFCE_SCREEN_POSITION_FLOATING_V:
		switch (xfapplet_panel_near (GTK_WIDGET (plugin), GTK_ORIENTATION_VERTICAL)) {
		case PANEL_NEAR_RIGHT:
			return GNOME_APPLET_ORIENT_LEFT;
		case PANEL_NEAR_LEFT:
			return GNOME_APPLET_ORIENT_RIGHT;
		default:
			g_assert_not_reached ();
			break;
		}
		break;
	case XFCE_SCREEN_POSITION_NONE:
		break;
        default:
                g_assert_not_reached ();
                break;
        }

	return GNOME_APPLET_ORIENT_UP;
}

static unsigned short
xfapplet_xfce_size_to_gnome_size (gint size)
{
	unsigned short sz;
	
	sz = size <= GNOME_PANEL_SIZE_XX_SMALL ? GNOME_PANEL_SIZE_XX_SMALL :
	     size <= GNOME_PANEL_SIZE_X_SMALL  ? GNOME_PANEL_SIZE_X_SMALL  :
	     size <= GNOME_PANEL_SIZE_SMALL    ? GNOME_PANEL_SIZE_SMALL    :
	     size <= GNOME_PANEL_SIZE_MEDIUM   ? GNOME_PANEL_SIZE_MEDIUM   :
	     size <= GNOME_PANEL_SIZE_LARGE    ? GNOME_PANEL_SIZE_LARGE    :
	     size <= GNOME_PANEL_SIZE_X_LARGE  ? GNOME_PANEL_SIZE_X_LARGE  : GNOME_PANEL_SIZE_XX_LARGE;

	return sz;
}

static void
xfapplet_screen_position_changed (XfcePanelPlugin *plugin, XfceScreenPosition position, gpointer data)
{
	unsigned short	 orientation;
	GtkWidget	*child = NULL;
	XfAppletPlugin	*xap = (XfAppletPlugin*) data;

	if (!xap->configured)
		return;

	/*
	 * Workaround: Xfce panel makes all items in all panels insensitive when
	 * moving items between panels. Some Gnome applets (noticeably the Mixer
	 * applet) misbehave when the following happens:
	 * 1. Applet is set insensitive;
	 * 2. Applet changes orientation;
	 * 3. Applet is set sensitive again.
	 * The following makes sure the applet is set sensitive before it
	 * changes orientation.
	 */

	child = xfapplet_get_plugin_child (xap->plugin);
	if (child)
		gtk_widget_set_sensitive (child, TRUE);

	orientation = xfapplet_xfce_screen_position_to_gnome_applet_orientation (plugin, position);
	bonobo_pbclient_set_short (xap->prop_bag, "panel-applet-orient", orientation, NULL);

	gtk_widget_queue_resize (GTK_WIDGET (plugin));
}

static gboolean 
xfapplet_size_changed (XfcePanelPlugin *plugin, int size, gpointer data)
{
	XfAppletPlugin	*xap = (XfAppletPlugin*) data;
	
	if (xap->configured) {
		gint sz = xfapplet_xfce_size_to_gnome_size (size);
		bonobo_pbclient_set_short (xap->prop_bag, "panel-applet-size", sz, NULL);
	}

	if (xfce_panel_plugin_get_orientation (plugin) == GTK_ORIENTATION_HORIZONTAL)
		gtk_widget_set_size_request (GTK_WIDGET (plugin), -1, size);
	else
		gtk_widget_set_size_request (GTK_WIDGET (plugin), size, -1);

	return TRUE;
}

static const gchar*
xfapplet_get_orient_string (XfcePanelPlugin *plugin)
{
	XfceScreenPosition	 position;
	const gchar		*ret = NULL;

	position = xfce_panel_plugin_get_screen_position (plugin);

	switch (xfapplet_xfce_screen_position_to_gnome_applet_orientation (plugin, position)) {
	case GNOME_APPLET_ORIENT_UP:
		ret = "up";
		break;
	case GNOME_APPLET_ORIENT_DOWN:
		ret = "down";
		break;
	case GNOME_APPLET_ORIENT_RIGHT:
		ret = "right";
		break;
	case GNOME_APPLET_ORIENT_LEFT:
		ret = "left";
		break;
	default:
		g_assert_not_reached ();
	}

	return ret;
}

static const gchar*
xfapplet_get_size_string (XfcePanelPlugin *plugin)
{
	gint		 size;
	const gchar	*retval;

	size = xfce_panel_plugin_get_size (plugin);
	size = xfapplet_xfce_size_to_gnome_size (size);

        switch (size) {
	case GNOME_PANEL_SIZE_XX_SMALL:
                retval = "xx-small";
		break;
	case GNOME_PANEL_SIZE_X_SMALL:
		retval = "x-small";
		break;
	case GNOME_PANEL_SIZE_SMALL:
                retval = "small";
		break;
	case GNOME_PANEL_SIZE_MEDIUM:
                retval = "medium";
		break;
	case GNOME_PANEL_SIZE_LARGE:
                retval = "large";
		break;
	case GNOME_PANEL_SIZE_X_LARGE:
                retval = "x-large";
		break;
	default:
                retval = "xx-large";
		break;
	}

        return retval;
}

static gboolean
xfapplet_save_configuration (XfAppletPlugin *xap)
{
	XfceRc         *config;
	gchar          *path;

	if (!xap->configured)
		return FALSE;

	path = xfce_panel_plugin_lookup_rc_file (xap->plugin);
	if (!path)
		path = xfce_panel_plugin_save_location (xap->plugin, TRUE);

	if (!path)
		return FALSE;

        config = xfce_rc_simple_open (path, FALSE);
	g_free (path);

	if (!config)
		return FALSE;

	xfce_rc_set_group (config, "xfapplet");

	/* iid for bonobo control */
	xfce_rc_write_entry (config, "iid", xap->iid);

	/* applet name (used in dialog messages) */
	xfce_rc_write_entry (config, "name", xap->name);

	/* gconf key for applet preferences */
	xfce_rc_write_entry (config, "gconfkey", xap->gconf_key);

	xfce_rc_close (config);

	return TRUE;
}

static gboolean
xfapplet_read_configuration (XfAppletPlugin *xap)
{
	XfceRc		*config;
	gchar		*path;
	const gchar	*iid;
	const gchar	*name;
	const gchar	*gconf_key;

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

	/* applet name (used in dialog messages) */
	name = xfce_rc_read_entry (config, "name", NULL);

	/* gconf key for applet preferences */
	gconf_key = xfce_rc_read_entry (config, "gconfkey", NULL);

	if (!iid || !gconf_key || !name) {
		xfce_rc_close (config);
		return FALSE;
	}
	
	xap->iid = g_strdup (iid);
	xap->name = g_strdup (name);
	xap->gconf_key = g_strdup (gconf_key);

	xfce_rc_close (config);

	return TRUE;
}

static void
xfapplet_about_dialog_response (GtkDialog *dialog, gint dummy1, gpointer dummy2)
{
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
xfapplet_about_dialog (XfcePanelPlugin *plugin, gpointer data)
{
	XfceAboutInfo	*info;
	GtkWidget	*dialog;
	GdkPixbuf	*pixbuf = NULL;
	guint		 i;
	static const XfAppletTranslators translators[] = {
		{"Stephane Roy", "sroy@j2n.net", "fr"},
		{"SZERVÑC Attila", "sas@321.hu", "hu",},
		{"Daichi Kawahata", "daichi@xfce.org", "ja",},
		{"Vincent Tunru", "imnotb@gmail.com", "nl",},
		{"Adriano Winter Bess", "awbess@gmail.com", "pt_BR",},
		{"Phan Vĩnh Thịnh", "teppi@vnlinux.org", "vi",},
		{NULL,}
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

	pixbuf = gdk_pixbuf_new_from_file_at_size (DATADIR "/pixmaps/xfapplet2.svg", 48, 48, NULL);
	dialog = xfce_about_dialog_new_with_values (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
						    info, pixbuf);
	
	xfce_about_info_free (info);
	if (pixbuf)
		g_object_unref (pixbuf);

	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
	g_signal_connect (dialog, "response", G_CALLBACK (xfapplet_about_dialog_response), NULL);
	gtk_widget_show (dialog);
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
		bonobo_ui_component_add_verb (uic, "CustomizePanel", xfapplet_menu_item_activated, data);

	/* "Add New Item" menu item */
	data = g_list_nth_data (list, ADD_MENU_ITEM_ORDER);
	if (GTK_IS_MENU_ITEM (data))
		bonobo_ui_component_add_verb (uic, "Add", xfapplet_menu_item_activated, data);

	/* "Remove" menu item */
	data = g_list_nth_data (list, REMOVE_MENU_ITEM_ORDER);
	if (GTK_IS_MENU_ITEM (data))
		bonobo_ui_component_add_verb (uic, "Remove", xfapplet_menu_item_activated, data);

	/* "Move" menu item */
	data = g_list_nth_data (list, MOVE_MENU_ITEM_ORDER);
	if (GTK_IS_MENU_ITEM (data))
		bonobo_ui_component_add_verb (uic, "Move", xfapplet_menu_item_activated, data);
	/* "About" menu item */
	data = g_list_nth_data (list, ABOUT_MENU_ITEM_ORDER);
	if (GTK_IS_MENU_ITEM (data))
		bonobo_ui_component_add_verb (uic, "About", xfapplet_menu_item_activated, data);

	/* "Properties" menu item */
	data = g_list_nth_data (list, PROPERTIES_MENU_ITEM_ORDER);
	if (GTK_IS_MENU_ITEM (data))
		bonobo_ui_component_add_verb (uic, "Properties", xfapplet_menu_item_activated, data);
}

static void
xfapplet_failed_response (GtkDialog *dialog, gint dummy, XfcePanelPlugin *plugin)
{
	gtk_widget_destroy (GTK_WIDGET (dialog));
	xfce_panel_plugin_unblock_menu (plugin);
}

static void
xfapplet_applet_load_failed (XfAppletPlugin *xap)
{
	GtkWidget	*dialog;
	GdkScreen	*screen;

	screen = gtk_widget_get_screen (GTK_WIDGET (xap->plugin));
      	dialog = gtk_message_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (xap->plugin))),
					 GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_ERROR,
					 GTK_BUTTONS_CLOSE,
					 _("'%s' could not be loaded."),
					 xap->name);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						  _("An internal error occurred and the applet could not be loaded."));
	gtk_window_set_screen (GTK_WINDOW (dialog), screen);
	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
	g_signal_connect (dialog, "response", G_CALLBACK (xfapplet_failed_response), xap->plugin);
	gtk_widget_show (dialog);

	xfce_panel_plugin_block_menu (xap->plugin);
}

static void
xfapplet_applet_activated (Bonobo_Unknown object, CORBA_Environment *ev, gpointer data)
{
	GtkWidget		*bw, *child = NULL;
	CORBA_Object		 control;
	CORBA_Environment	 corba_ev;
	BonoboControlFrame	*frame;
	BonoboUIComponent	*uic;
	Bonobo_PropertyBag	 prop_bag;
	XfAppletPlugin		*xap = (XfAppletPlugin*) data;
	gchar			*error;

	if (BONOBO_EX (ev) || object == CORBA_OBJECT_NIL) {
		error = bonobo_exception_get_text (ev);
		CORBA_exception_free (ev);
		g_warning ("Failed to load applet '%s' (can't get CORBA object): %s\n", xap->iid, error);
		xfapplet_applet_load_failed (xap);
		xfapplet_cleanup_current (xap);
		g_free (error);
		return;
	}
	
	control = CORBA_Object_duplicate (object, NULL);
	bw = bonobo_widget_new_control_from_objref (object, CORBA_OBJECT_NIL);
	bonobo_object_release_unref (object, NULL);
	if (!bw) {
		g_warning ("Failed to load applet '%s' (can't get bonobo widget)\n", xap->iid);
		xfapplet_applet_load_failed (xap);
		xfapplet_cleanup_current (xap);
		return;
	}
	
	frame = bonobo_widget_get_control_frame (BONOBO_WIDGET (bw));
	if (!frame) {
		g_warning ("Failed to load applet '%s' (can't get control frame)\n", xap->iid);
		gtk_object_sink (GTK_OBJECT (bw));
		xfapplet_applet_load_failed (xap);
		xfapplet_cleanup_current (xap);
		return;
	}
	
	CORBA_exception_init (&corba_ev);
	prop_bag = bonobo_control_frame_get_control_property_bag (frame, &corba_ev);
	if (prop_bag == NULL || BONOBO_EX (&corba_ev)) {
		error = bonobo_exception_get_text (&corba_ev);
		CORBA_exception_free (&corba_ev);
		g_warning ("Failed to load applet '%s' (can't get property bag): %s\n", xap->iid, error);
		gtk_object_sink (GTK_OBJECT (bw));
		xfapplet_applet_load_failed (xap);
		xfapplet_cleanup_current (xap);
		g_free (error);
		return;
	}
	
        uic = bonobo_control_frame_get_popup_component (frame, &corba_ev);
	if (uic == NULL || BONOBO_EX (&corba_ev)) {
		error = bonobo_exception_get_text (&corba_ev);
		CORBA_exception_free (&corba_ev);
		g_warning ("Failed to load applet '%s' (can't get popup component): %s\n", xap->iid, error);
		gtk_object_sink (GTK_OBJECT (bw));
		xfapplet_applet_load_failed (xap);
		xfapplet_cleanup_current (xap);
		g_free (error);
		return;
	}
		
	bonobo_ui_component_freeze (uic, CORBA_OBJECT_NIL);
	xfce_textdomain ("xfce4-panel", LIBXFCE4PANEL_LOCALE_DIR, "UTF-8");
	bonobo_ui_util_set_ui (uic, PKGDATADIR "/ui", "XFCE_Panel_Popup.xml", "xfce4-xfapplet-plugin", &corba_ev);
	if (BONOBO_EX (&corba_ev)) {
		error = bonobo_exception_get_text (&corba_ev);
		CORBA_exception_free (&corba_ev);
		g_warning ("Failed to load applet '%s' (can't set ui): %s\n", xap->iid, error);
		gtk_object_sink (GTK_OBJECT (bw));
		xfapplet_applet_load_failed (xap);
		xfapplet_cleanup_current (xap);
		g_free (error);
		return;
	}
	xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
	xfapplet_setup_menu_items (xap->plugin, uic);
	bonobo_ui_component_thaw (uic, CORBA_OBJECT_NIL);

	gtk_widget_show (bw);

	if (xap->configured)
		xfapplet_unload_applet (xap);

	xap->object = control;
	xap->uic = uic;
	xap->prop_bag = prop_bag;
	ORBit_small_listen_for_broken (object, G_CALLBACK (xfapplet_connection_broken), xap);

	child = xfapplet_get_plugin_child (xap->plugin);
	if (child)
		gtk_widget_destroy (child);

	gtk_container_add (GTK_CONTAINER(xap->plugin), bw);
	xap->configured = TRUE;

	if (!xfapplet_save_configuration (xap))
		g_warning ("Failed to save XfApplet configuration.\n");
}

static void
xfapplet_free(XfcePanelPlugin *plugin, XfAppletPlugin *xap)
{
	GtkWidget *child;
	
	if (xap->configured) {
		xfapplet_unload_applet (xap);

		/*
		 * The following may not seem necessary, but indeed it is. If
		 * the plugin holds a bonobo widget and it is not explicitely
		 * destroyed here, an "applet freeze" bug shows up. This bug can
		 * be reproduced in the following steps:
		 * 1. Add two xfapplets;
		 * 2. Choose the same applet for both of them;
		 * 3. Remove one of the xfapplets;
		 * Result: the remaining applet will be frozen
		 */
		child = xfapplet_get_plugin_child (xap->plugin);
		if (child)
			gtk_widget_destroy (child);
	}

	xfapplet_cleanup_current (xap);

	xfapplet_cleanup_unused_gconf_keys ();

	g_free (xap);
}

static gchar*
xfapplet_construct_moniker (XfAppletPlugin *xap)
{
	gchar *moniker;

	/* We (still) do not support "background" and "locked_down" options. */
	moniker = g_strdup_printf ("%s!prefs_key=%s/prefs;orient=%s;size=%s",
				   xap->iid, xap->gconf_key,
				   xfapplet_get_orient_string (xap->plugin),
				   xfapplet_get_size_string (xap->plugin));

	return moniker;
}

void
xfapplet_setup_full (XfAppletPlugin *xap)
{
	CORBA_Environment	 ev;
	gchar			*moniker;

	CORBA_exception_init (&ev);
	moniker = xfapplet_construct_moniker (xap);
	bonobo_get_object_async (moniker, "IDL:Bonobo/Control:1.0", &ev, xfapplet_applet_activated, xap);
	g_free (moniker);
}

static void
xfapplet_setup_empty (XfAppletPlugin *xap)
{
	GtkWidget *img, *eb, *child = NULL;
	GdkPixbuf *pixbuf;
	gint	   size;
	
	size = xfce_panel_plugin_get_size (xap->plugin);
	pixbuf = gdk_pixbuf_new_from_file_at_size (DATADIR "/pixmaps/xfapplet1.svg", size, size, NULL);
	img = gtk_image_new_from_pixbuf (pixbuf);
	g_object_unref (pixbuf);
	gtk_widget_show (img);

	eb = gtk_event_box_new ();
	gtk_container_add (GTK_CONTAINER (eb), img);
	gtk_widget_show (eb);

	child = xfapplet_get_plugin_child (xap->plugin);
	if (child)
		gtk_widget_destroy (child);
	
	gtk_container_add (GTK_CONTAINER(xap->plugin), eb);
	xfce_panel_plugin_add_action_widget (xap->plugin, eb);
	xfce_panel_plugin_menu_show_configure (xap->plugin);
	xfce_panel_plugin_menu_show_about (xap->plugin);

	xap->configured = FALSE;
}

static XfAppletPlugin*
xfapplet_new (XfcePanelPlugin *plugin)
{
	XfAppletPlugin *xap;

	xap = g_new0 (XfAppletPlugin, 1);
	xap->plugin = plugin;
	xap->configured = FALSE;

	return xap;
}

static void
xfapplet_construct (XfcePanelPlugin *plugin)
{
	int		 argc = 1;
	char		*argv[] = { "xfce4-xfapplet-plugin", };
	XfAppletPlugin	*xap;

	xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

	bonobo_ui_init (argv[0], VERSION, &argc, argv);

	xfapplet_cleanup_unused_gconf_keys ();

	xap = xfapplet_new (plugin);

	xfapplet_setup_empty (xap);
	
	if (xfapplet_read_configuration (xap))
		xfapplet_setup_full (xap);

	g_signal_connect (plugin, "free-data", G_CALLBACK (xfapplet_free), xap);
	g_signal_connect (plugin, "size-changed", G_CALLBACK (xfapplet_size_changed), xap);
	g_signal_connect (plugin, "screen-position-changed", G_CALLBACK (xfapplet_screen_position_changed), xap);
	g_signal_connect (plugin, "configure-plugin", G_CALLBACK (xfapplet_chooser_dialog), xap);
	g_signal_connect (plugin, "about", G_CALLBACK (xfapplet_about_dialog), xap);
}

XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL(xfapplet_construct)

