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

#include <string.h>
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>
#include <bonobo/bonobo-exception.h>
#include <bonobo-activation/bonobo-activation.h>
#include <libxfcegui4/libxfcegui4.h>
#include "xfapplet.h"

static const char applets_requirements [] =
"has_all (repo_ids, ['IDL:Bonobo/Control:1.0',"
"		     'IDL:GNOME/Vertigo/PanelAppletShell:1.0']) && "
"defined (panel:icon)";

static char *applets_sort_criteria [] = {
	"name",
	NULL
};

typedef struct {
	gchar     *name;
	gchar     *description;
	GdkPixbuf *icon;
	gchar     *iid;	
} GnomeAppletInfo;

typedef struct {
	GtkWidget	*tv;
	GSList		*applets;
	gulong		 destroy_id;
	XfAppletPlugin	*xap;
} XfAppletChooserDialog;

static gchar *
xfapplet_find_unique_key (gchar *applet_name)
{
	GConfClient	*client;
	gchar		*key = NULL;
	gchar		*in_use;
	gchar		*name;
	int		 i = 0;

	client = gconf_client_get_default ();

	do {
		g_free (key);
		key = g_strdup_printf (XFAPPLET_GCONF_DIR "applet_%d", i++);
	} while (gconf_client_dir_exists (client, key, NULL));

	in_use = g_strdup_printf ("%s/in_use", key);
	gconf_client_set_bool (client, in_use, TRUE, NULL);
	g_free (in_use);

	name = g_strdup_printf ("%s/name", key);
	gconf_client_set_string (client, name, applet_name, NULL);
	g_free (name);

	g_object_unref (client);

	return key;
}

/*
 * This one was borrowed (almost) entirely from the Gnome panel. Since
 * each applet specifies its icon in a different way, we have to fix
 * things up.
 */
static gchar *
xfapplet_find_icon (const gchar *icon_name, gint size)
{
	GtkIconTheme *icon_theme;
	GtkIconInfo  *info;
	char         *retval;
	char         *icon_no_extension;
	char         *p;
	gchar        **path;
	gint          nitems;
	gchar        *relative;

	if (icon_name == NULL || strcmp (icon_name, "") == 0)
		return NULL;

	icon_theme = gtk_icon_theme_get_default ();

	if (g_path_is_absolute (icon_name)) {
		if (g_file_test (icon_name, G_FILE_TEST_EXISTS)) {
			return g_strdup (icon_name);
		} else {
			char *basename;

			basename = g_path_get_basename (icon_name);
			retval = xfapplet_find_icon (basename, size);
			g_free (basename);

			return retval;
		}
	}

	/* This is needed because some .desktop files have an icon name *and*
	 * an extension as icon */
	icon_no_extension = g_strdup (icon_name);
	p = strrchr (icon_no_extension, '.');
	if (p &&
	    (strcmp (p, ".png") == 0 ||
	     strcmp (p, ".xpm") == 0 ||
	     strcmp (p, ".svg") == 0)) {
	    *p = 0;
	}
	
	/*
	 * Some applets provide a relative path (noticeably the Linphone
	 * applet), but gtk_icon_theme_lookup_icon() has trouble with this, so
	 * we must also fix it.
	 */
	relative = g_path_get_dirname (icon_no_extension);
	if (strcmp (relative, ".") != 0) {
		gint   i;
		gchar *icon_no_relative;

		icon_no_relative = g_path_get_basename (icon_no_extension);
		g_free (icon_no_extension);
		icon_no_extension = icon_no_relative;
		gtk_icon_theme_get_search_path (icon_theme, &path, &nitems);
		for (i = 0; i < nitems; i++) {
			gchar *newpath = g_build_filename (path[i], relative, NULL);
			gtk_icon_theme_prepend_search_path (icon_theme, newpath);
			g_free (newpath);
		}
	}
	else {
		g_free (relative);
		relative = NULL;
	}

	info = gtk_icon_theme_lookup_icon (icon_theme, icon_no_extension, size, 0);
	g_free (icon_no_extension);

	if (info) {
		retval = g_strdup (gtk_icon_info_get_filename (info));
		gtk_icon_info_free (info);
	} else
		retval = NULL;

	if (relative) {
		gtk_icon_theme_set_search_path (icon_theme, (const gchar**)path, nitems);
		g_strfreev (path);
		g_free (relative);
	}
	
	return retval;
}

static void
xfapplet_fill_model (GSList *list, GtkListStore *store, GtkWidget *scroll, GtkWidget *tv)
{
	GtkTreeIter iter;
	guint       i = 0;
	
	for (; list; list = list->next, i++) {
		if (i == 5) {
			GtkRequisition req;

			gtk_widget_size_request (tv, &req);
			gtk_widget_set_size_request (tv, -1, req.height);

			gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
							GTK_POLICY_NEVER, 
							GTK_POLICY_ALWAYS);
		}

		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, list->data, -1);
	}

	if (i < 5) {
		int desired_width, desired_height;

		if (!gtk_icon_size_lookup (GTK_ICON_SIZE_DIALOG, &desired_width, &desired_height))
			gtk_widget_set_size_request (tv, -1, 240);
		else 
			gtk_widget_set_size_request (tv, -1, 5*desired_height);
	}
}

static GnomeAppletInfo*
xfapplet_read_applet_info (Bonobo_ServerInfo *info, GSList *langs)
{
	const char         *name, *description, *icon;
	GnomeAppletInfo    *applet;
	GdkPixbuf          *pb;
	int                 desired_width, desired_height;

	name = bonobo_server_info_prop_lookup (info, "name", langs);
	description = bonobo_server_info_prop_lookup (info, "description", langs);
	icon = bonobo_server_info_prop_lookup (info, "panel:icon", NULL);

	if (!name)
		return NULL;

	applet = g_new0 (GnomeAppletInfo, 1);
	applet->name = g_strdup (name);
	applet->description = g_strdup (description);
	applet->iid = g_strdup (info->iid);

	if (!gtk_icon_size_lookup (GTK_ICON_SIZE_DIALOG, &desired_width, &desired_height))
		applet->icon = NULL;
	else {
		gchar *iconfile = xfapplet_find_icon (icon, desired_height);
		if (iconfile) {
			pb = gdk_pixbuf_new_from_file_at_size (iconfile, desired_width,
							       desired_height, NULL);
			if (!pb)
				applet->icon = NULL;
			else
				applet->icon = pb;
			g_free (iconfile);
		}
		else
			applet->icon = NULL;
	}

	return applet;
}

static GSList*
xfapplet_query_applets ()
{
	Bonobo_ServerInfoList *applet_list;
	CORBA_Environment      env;
	const char * const    *langs;
	GSList                *langs_gslist, *list = NULL;
	int                    i;

	CORBA_exception_init (&env);
	applet_list = bonobo_activation_query (applets_requirements, applets_sort_criteria, &env);
	if (BONOBO_EX (&env)) {
		g_warning ("Failed to query applets: %s\n", BONOBO_EX_REPOID (&env));
		CORBA_exception_free (&env);
		CORBA_free (applet_list);
		return NULL;
	}
	CORBA_exception_free (&env);

	langs = g_get_language_names ();
	langs_gslist = NULL;
	for (i = 0; langs[i]; i++)
		langs_gslist = g_slist_prepend (langs_gslist, (char *) langs[i]);
	langs_gslist = g_slist_reverse (langs_gslist);

	for (i = 0; i < applet_list->_length; i++) {
		GnomeAppletInfo *applet = xfapplet_read_applet_info (&applet_list->_buffer[i], langs_gslist);
		if (applet)
			list = g_slist_prepend (list, applet);
	}

	g_slist_free (langs_gslist);
	CORBA_free (applet_list);

	return list;
}

static void
xfapplet_render_icon (GtkTreeViewColumn *col, GtkCellRenderer *cell,
		      GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	GnomeAppletInfo *applet;

	gtk_tree_model_get (model, iter, 0, &applet, -1);

	if (applet)
		g_object_set (cell, "pixbuf", applet->icon, NULL);
	else
		g_object_set (cell, "pixbuf", NULL, NULL);
}

static void
xfapplet_render_text (GtkTreeViewColumn *col, GtkCellRenderer *cell,
		      GtkTreeModel *model, GtkTreeIter *iter, GtkWidget *treeview)
{
	GnomeAppletInfo *applet;
	gchar           *text;

	gtk_tree_model_get (model, iter, 0, &applet, -1);

	if (applet) {
		if (applet->description)
			text = g_strdup_printf ("<b>%s</b>\n%s", applet->name, applet->description);
		else
			text = g_strdup_printf ("<b>%s</b>", applet->name);
		g_object_set (cell, "markup", text, "foreground-set", FALSE, NULL);
		g_free (text);
	}
	else
		g_object_set (cell, "markup", "", "foreground-set", TRUE, NULL);
}

void
xfapplet_free_applet_info (GnomeAppletInfo *applet)
{
	if (applet) {
		g_free (applet->name);
		g_free (applet->description);
		g_free (applet->iid);
		if (applet->icon)
			g_object_unref (applet->icon);
		g_free (applet);
	}
}

static void
xfapplet_free_applet_list (GSList *lst)
{
	GSList          *list;

	for (list = lst; list; list = list->next) {
		if (list->data) 
			xfapplet_free_applet_info ((GnomeAppletInfo*) list->data);
	}

	g_slist_free (lst);
}

static void
xfapplet_chooser_dialog_response (GtkWidget *dialog, int response, XfAppletChooserDialog *chooser)
{
	GtkTreeSelection	*sel;
	GtkTreeModel		*model;
	GtkTreeIter		 iter;
	GnomeAppletInfo		*applet;
	XfAppletPlugin		*xap = chooser->xap;

	if (response == GTK_RESPONSE_OK) {
		sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (chooser->tv));
		if (gtk_tree_selection_get_selected (sel, &model, &iter)) {
			gtk_tree_model_get (model, &iter, 0, &applet, -1);
			xfapplet_cleanup_current (xap);
			xap->iid = g_strdup (applet->iid);
			xap->name = g_strdup (applet->name);
			xap->gconf_key = xfapplet_find_unique_key (applet->name);
			xfapplet_setup_full (xap);
		}
	}

	g_signal_handler_disconnect (dialog, chooser->destroy_id);	
	gtk_widget_destroy (dialog);
	xfapplet_free_applet_list (chooser->applets);
	xfce_panel_plugin_unblock_menu (xap->plugin);
	g_free (chooser);
}

static void
xfapplet_treeview_destroyed (GtkWidget * tv)
{
	GtkTreeModel *store;

	store = gtk_tree_view_get_model (GTK_TREE_VIEW (tv));
	gtk_list_store_clear (GTK_LIST_STORE (store));
}

static int
xfapplet_applet_compare (GnomeAppletInfo *a, GnomeAppletInfo *b)
{
	return g_utf8_collate (a->name, b->name);
}


static void
xfapplet_label_resized (GtkWidget *label, GtkAllocation *alloc, gpointer dummy)
{
	gtk_widget_set_size_request (label, alloc->width, -1);
}

static void
xfapplet_chooser_destroyed (GtkDialog *dialog)
{
	gtk_dialog_response (dialog, GTK_RESPONSE_CANCEL);
}

static gboolean
xfapplet_tv_double_click (GtkWidget *tv, GdkEventButton *event, gpointer dialog)
{
	if (event->button == 1 && event->type == GDK_2BUTTON_PRESS) {
		gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
		return TRUE;
	}

	return FALSE;
}

void
xfapplet_chooser_dialog (XfcePanelPlugin *plugin, XfAppletPlugin *xap)
{
	XfAppletChooserDialog	*chooser;
	GtkWidget		*dialog, *label, *scroll, *ok_button,
				*tv, *img, *header, *hbox, *info_label;
	GtkBox			*vbox;
	GtkTreeViewColumn	*col;
	GtkTreeSelection	*sel;
	GtkCellRenderer		*cell;
	GtkListStore		*store;
	GtkTreeModel		*model;
	GtkTreePath		*path;
	GSList			*list = NULL;
	GdkColor		*color;
	gchar			*markup;
	gulong			 signal;

	xfce_panel_plugin_block_menu (plugin);

	/* creation of the applet chooser structure */
	chooser = g_new0 (XfAppletChooserDialog, 1);
	chooser->xap = xap;

	/* creation of the dialog */
	dialog = gtk_dialog_new_with_buttons (_("XfApplet"),
					      GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
					      GTK_DIALOG_DESTROY_WITH_PARENT|GTK_DIALOG_NO_SEPARATOR,
					      GTK_STOCK_CANCEL,
					      GTK_RESPONSE_CANCEL,
					      NULL);
	ok_button = gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_OK, GTK_RESPONSE_OK);
	gtk_widget_set_sensitive (ok_button, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 8);
	g_signal_connect (dialog, "response", G_CALLBACK (xfapplet_chooser_dialog_response), chooser);
	chooser->destroy_id = g_signal_connect_swapped (dialog, "destroy",
							G_CALLBACK (xfapplet_chooser_destroyed), chooser);
	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);

	/* grab the dialog vbox to fill it with our stuff */
	vbox = GTK_BOX (GTK_DIALOG (dialog)->vbox);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
	gtk_box_set_spacing (vbox, 8);

	/* dialog header */
	img = gtk_image_new_from_icon_name ("xfapplet2", GTK_ICON_SIZE_DIALOG);
	header = xfce_create_header_with_image (img, _("Choose an applet"));
	gtk_box_pack_start (vbox, header, FALSE, FALSE, 0);

	/* dialog info message */
	hbox = gtk_hbox_new (FALSE, 8);
	gtk_box_pack_start (vbox, hbox, TRUE, TRUE, 0);
	img = gtk_image_new_from_stock (GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_misc_set_alignment (GTK_MISC (img), 0, 0);
	gtk_box_pack_start (GTK_BOX (hbox), img, FALSE, FALSE, 0);
	label = gtk_label_new (_("Choose an applet from the list. If you have already chosen an "
				 "applet previously, it will be substituted by the one you choose."));
	signal = g_signal_connect (label, "size-allocate", G_CALLBACK (xfapplet_label_resized), NULL);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
	info_label = label;

	/* treeview "title" */
	label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_box_pack_start (vbox, label, FALSE, FALSE, 0);
	markup = g_strdup_printf ("<b>%s</b>", _("Available applets"));
	gtk_label_set_markup (GTK_LABEL (label), markup);
	g_free (markup);
	
	/* create and setup treeview with applets */
	scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_NEVER, GTK_POLICY_NEVER);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll), GTK_SHADOW_IN);
	gtk_box_pack_start (vbox, scroll, TRUE, TRUE, 0);
    
	store = gtk_list_store_new (1, G_TYPE_POINTER);
	model = GTK_TREE_MODEL (store);

	tv = gtk_tree_view_new_with_model (model);
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (tv), TRUE);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tv), FALSE);
	gtk_container_add (GTK_CONTAINER (scroll), tv);
	chooser->tv = tv;

	g_signal_connect (tv, "destroy", G_CALLBACK (xfapplet_treeview_destroyed), NULL);
	g_object_unref (G_OBJECT (store));

	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_spacing (col, 8);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tv), col);

	cell = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (col, cell, FALSE);
	gtk_tree_view_column_set_cell_data_func (col, cell, (GtkTreeCellDataFunc) xfapplet_render_icon, NULL, NULL);

	cell = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (col, cell, TRUE);
	gtk_tree_view_column_set_cell_data_func (col, cell, (GtkTreeCellDataFunc) xfapplet_render_text, tv, NULL);

	color = &(tv->style->fg[GTK_STATE_INSENSITIVE]);
	g_object_set (cell, "foreground-gdk", color, NULL);

	list = xfapplet_query_applets ();
	list = g_slist_sort (list, (GCompareFunc) xfapplet_applet_compare);
	xfapplet_fill_model (list, store, scroll, tv);
	chooser->applets = list;
	
	/* select first applet in the treeview */
	path = gtk_tree_path_new_from_string ("0");
	gtk_tree_view_set_cursor (GTK_TREE_VIEW (tv), path, NULL, FALSE);
	gtk_tree_path_free (path);
	
	/* 
	 * If there's at least one applet, allow user to load it by the OK
	 * button and by double click.
	 */
	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (chooser->tv));
	if (gtk_tree_selection_get_selected (sel, NULL, NULL)) {
		gtk_widget_set_sensitive (ok_button, TRUE);
		g_signal_connect (tv, "button-press-event", G_CALLBACK (xfapplet_tv_double_click), dialog);
	}

	gtk_widget_show_all (dialog);
	g_signal_handler_disconnect (info_label, signal);
}

