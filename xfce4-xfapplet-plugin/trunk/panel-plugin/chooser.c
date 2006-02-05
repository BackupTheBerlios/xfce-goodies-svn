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
#include <libbonobo.h>
#include "xfapplet.h"

static const char applets_requirements [] =
"has_all (repo_ids, ['IDL:Bonobo/Control:1.0',"
"		     'IDL:GNOME/Vertigo/PanelAppletShell:1.0']) && "
"defined (panel:icon)";

static char *applets_sort_criteria [] = {
	"name",
	NULL
};

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
	info = gtk_icon_theme_lookup_icon (icon_theme, icon_no_extension, size, 0);
	g_free (icon_no_extension);

	if (info) {
		retval = g_strdup (gtk_icon_info_get_filename (info));
		gtk_icon_info_free (info);
	} else
		retval = NULL;

	return retval;
}

static void
xfapplet_fill_model (GSList *list, GtkListStore *store, GtkWidget *scroll, GtkWidget *tv)
{
	GtkTreeIter iter;
	guint       i = 0;
	
	for (; list; list = list->next, i++) {
		if (i == 8) {
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
		g_warning (_("query returned exception %s\n"),
			   BONOBO_EX_REPOID (&env));

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
		Bonobo_ServerInfo  *info;
		const char         *name, *description, *icon;
		GnomeAppletInfo    *applet;
		GdkPixbuf          *pb;
		int                 desired_width, desired_height;

		info = &applet_list->_buffer[i];
		name = bonobo_server_info_prop_lookup (info, "name", langs_gslist);
		description = bonobo_server_info_prop_lookup (info, "description", langs_gslist);
		icon = bonobo_server_info_prop_lookup (info, "panel:icon", NULL);

		if (!name)
			continue;

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

static void
xfapplet_free_applet_list (GSList *lst)
{
	GSList          *list;
	GnomeAppletInfo *applet;

	for (list = lst; list; list = list->next) {
		applet = list->data;
		if (applet) {
			g_free (applet->name);
			g_free (applet->description);
			g_free (applet->iid);
			if (applet->icon)
				g_object_unref (applet->icon);
			g_free (applet);
		}
	}

	g_slist_free (lst);
}

static void
xfapplet_chooser_dialog_response (GtkWidget *dialog, int response, XfAppletPlugin *xap)
{
	GtkTreeSelection *sel;
	GtkTreeModel     *model;
	GtkTreeIter       iter;
	GnomeAppletInfo  *applet;

	if (response == GTK_RESPONSE_OK) {
		sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (xap->tv));
		gtk_tree_selection_get_selected (sel, &model, &iter);
		gtk_tree_model_get (model, &iter, 0, &applet, -1);

		xap->moniker = g_strdup (applet->iid);
		xfapplet_setup_full (xap);
	}
	
	gtk_widget_destroy (dialog);
	xfapplet_free_applet_list (xap->applets);
	xfce_panel_plugin_unblock_menu (xap->plugin);
}

static void
xfapplet_treeview_destroyed (GtkWidget * tv)
{
    GtkTreeModel *store;

    store = gtk_tree_view_get_model (GTK_TREE_VIEW (tv));
    gtk_list_store_clear (GTK_LIST_STORE (store));
}

void
xfapplet_chooser_dialog (XfcePanelPlugin *plugin, XfAppletPlugin *xap)
{
	GtkWidget         *dialog;
	GtkWidget         *label;
	GtkWidget         *scroll;
	GtkWidget         *tv;
	GtkTreeViewColumn *col;
	GtkCellRenderer   *cell;
	GtkListStore      *store;
	GtkTreeModel      *model;
	GtkTreePath       *path;
	GSList            *list = NULL;
	GdkColor          *color;

	xfce_panel_plugin_block_menu (plugin);

	dialog = gtk_dialog_new_with_buttons ("Choose an applet",
					      GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
					      GTK_DIALOG_DESTROY_WITH_PARENT,
					      GTK_STOCK_CANCEL,
					      GTK_RESPONSE_CANCEL,
					      GTK_STOCK_OK,
					      GTK_RESPONSE_OK,
					      NULL);

	gtk_container_set_border_width (GTK_CONTAINER (dialog), 2);
	g_signal_connect (dialog, "response", G_CALLBACK (xfapplet_chooser_dialog_response), xap);
	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);

	label = gtk_label_new ("Choose an applet:");
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), label, FALSE, FALSE, 2);
	
	scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_NEVER, GTK_POLICY_NEVER);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll), GTK_SHADOW_IN);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), scroll, TRUE, TRUE, 0);
    
	store = gtk_list_store_new (1, G_TYPE_POINTER);
	model = GTK_TREE_MODEL (store);

	tv = gtk_tree_view_new_with_model (model);
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (tv), TRUE);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tv), FALSE);
	gtk_container_add (GTK_CONTAINER (scroll), tv);
	xap->tv = tv;

	g_signal_connect (tv, "destroy", G_CALLBACK (xfapplet_treeview_destroyed), NULL);
	g_object_unref (G_OBJECT (store));

	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_spacing (col, 8);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tv), col);

	cell = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (col, cell, FALSE);
	gtk_tree_view_column_set_cell_data_func (col, cell, (GtkTreeCellDataFunc) xfapplet_render_icon,
						 NULL, NULL);

	cell = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (col, cell, TRUE);
	gtk_tree_view_column_set_cell_data_func (col, cell, (GtkTreeCellDataFunc) xfapplet_render_text,
						 tv, NULL);

	color = &(tv->style->fg[GTK_STATE_INSENSITIVE]);
	g_object_set (cell, "foreground-gdk", color, NULL);

	list = xfapplet_query_applets ();
	xfapplet_fill_model (list, store, scroll, tv);
	xap->applets = list;

	path = gtk_tree_path_new_from_string ("0");
	gtk_tree_view_set_cursor (GTK_TREE_VIEW (tv), path, NULL, FALSE);
	gtk_tree_path_free (path);

	gtk_widget_show_all (dialog);
}
