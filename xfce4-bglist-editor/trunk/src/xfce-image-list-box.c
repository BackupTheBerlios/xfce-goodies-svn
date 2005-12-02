/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Xfce Background List Editor
 * Copyright (C) 2004 Danny Milosavljevic <danny_milo@yahoo.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#define TEST_IMAGE_FILES_BEFORE_ADD 1
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include "trace.h"

#include <gtk/gtk.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4util/libxfce4util.h>

#include "xfce-image-list-box.h"
#include "xfce-cell-renderer-pixbuf-on-demand.h"
#include "xfce-tree-helpers.h"

#define MAX_DIR_DEPTH 200
#define MAX_TITLE_LEN 80

enum {
	TARGET_URI = 0,
	TARGET_STRING = 1
};

static GtkTargetEntry targets[] = {
	{"text/uri-list", 0, TARGET_URI},
	{"text/plain", 0, TARGET_STRING},
	{"UTF8_STRING", 0, TARGET_STRING},
	{"text/unicode", 0, TARGET_STRING}, /* netscape... todo: test */
	{"STRING", 0, TARGET_STRING}
	/*("GTK_TREE_MODEL_ROW", gtk.TARGET_SAME_WIDGET, 0)*/                                        	
};

enum {
	SELECTION_CHANGED_SIGNAL,
	LAST_SIGNAL
};

enum {
	CHANGED_PROP = 1,
	FNAME_PROP = 2,
	LAST_PROP = 2
};

#define C_THUMB 0
#define C_NAME 1
#define C_PATH 2
#define C_LOADED 3
#define C_LOADED_DETAIL 4

struct _XfceImageListBoxPrivate
{
	GtkListStore *store;
	GtkButton *ok_button;
	GtkTreeViewColumn *thumb_column;
	XfceCellRendererPixbufOnDemand *thumb_renderer;
	gchar *list_fname; /* file name of the image list file */
	
	/* settings */
	gint thumb_width;
	gint thumb_height;
	
	/* misc */
	GtkAccelGroup *ag;
	
	/* widgets */
	GtkTreeView *tree;
	GtkVButtonBox *vbuttons;
	GtkButton *add_button;
	GtkButton *remove_button;
	GtkButton *up_button;
	GtkButton *down_button;
	GtkBox *left_box;
	GtkScrolledWindow *scroller;
	
	/* state */
	gboolean changed;
	
	GList *txtl; /* DND */
	GHashTable *existing_paths;
	
	gint check_files; /* "use count" */
};

static GtkHBoxClass *parent_class = NULL;

static guint XfceImageListBox_signals[LAST_SIGNAL] = { 0 };

static void xfce_image_list_box_class_init(XfceImageListBoxClass *klass);
static void xfce_image_list_box_init(XfceImageListBox *aXfceImageListBox);
static void xfce_image_list_box_finalize(GObject *object);
static void xfce_image_list_box_dispose(GObject *object);

static void xfce_image_list_box_get_property(GObject *object, guint param_id, GValue *value, GParamSpec *pspec);
static void xfce_image_list_box_set_property(GObject *object, guint param_id, const GValue *value, GParamSpec *pspec);

GType
xfce_image_list_box_get_type(void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo our_info =
			{
				sizeof(XfceImageListBoxClass),
				NULL,	   /* base_init */
				NULL,	   /* base_finalize */
				(GClassInitFunc) xfce_image_list_box_class_init,
				NULL,	   /* class_finalize */
				NULL,	   /* class_data */
				sizeof(XfceImageListBox),
				0,	      /* n_preallocs */
				(GInstanceInitFunc) xfce_image_list_box_init
			};
		
		type = g_type_register_static(GTK_TYPE_HBOX,
					      "XfceImageListBox",
					      &our_info,
					      0);
	}
	
	return type;
}

static gboolean
remove_hash_all_cb (gpointer key, gpointer value, gpointer user_data)
{
	return TRUE;
}
 

static void 
xfce_image_list_box_finalize(GObject *object)
{
	XfceImageListBox *aXfceImageListBox;
	XfceImageListBoxPrivate *priv;

	g_return_if_fail(object != NULL);
	g_return_if_fail(IS_XFCE_IMAGE_LIST_BOX(object));

	aXfceImageListBox = XFCE_IMAGE_LIST_BOX(object);
	
	priv = 	aXfceImageListBox->priv;
	if (priv->ag) {
		g_object_unref (G_OBJECT (priv->ag));
		priv->ag = NULL;
	}
	
	if (priv->existing_paths) {
		g_hash_table_foreach_remove (priv->existing_paths,
			remove_hash_all_cb,
			NULL
		);
		g_hash_table_destroy (priv->existing_paths);
		priv->existing_paths = NULL;
	}
	
	g_free(priv);

	if (G_OBJECT_CLASS(parent_class)->finalize)
	        (* G_OBJECT_CLASS(parent_class)->finalize)(object);

}
static void 
xfce_image_list_box_dispose(GObject *object)
{
	XfceImageListBox *aXfceImageListBox;

	g_return_if_fail(object != NULL);
	g_return_if_fail(IS_XFCE_IMAGE_LIST_BOX(object));

	aXfceImageListBox = XFCE_IMAGE_LIST_BOX(object);

	if (G_OBJECT_CLASS(parent_class)->dispose)
	        (* G_OBJECT_CLASS(parent_class)->dispose)(object);
}
static void
xfce_image_list_box_get_property(GObject *object, guint param_id, GValue *value, GParamSpec *pspec)
{
	XfceImageListBox *aXfceImageListBox;        
	XfceImageListBoxPrivate *priv;

	aXfceImageListBox = XFCE_IMAGE_LIST_BOX(object);

	priv = aXfceImageListBox->priv;
	switch (param_id) {
	case CHANGED_PROP:
		g_value_set_boolean (value, priv->changed);
		break;
		
	case FNAME_PROP:
		g_value_set_string (value, priv->list_fname);
		break;
		
	default:
	        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
	        break;
	}
}
static void
xfce_image_list_box_set_property(GObject *object, guint param_id, const GValue *value, GParamSpec *pspec)
{
	XfceImageListBox *aXfceImageListBox;        
	XfceImageListBoxPrivate *priv;
	
	gchar *fname;
	gchar const *fnamec;

	aXfceImageListBox = XFCE_IMAGE_LIST_BOX(object);
	priv = aXfceImageListBox->priv;

	switch (param_id) {
	case FNAME_PROP:
		if (priv->list_fname) {
			g_free (priv->list_fname);
			priv->list_fname = NULL;
		}
		fnamec = g_value_get_string (value);
		if (fnamec) 
			priv->list_fname = g_strdup (fnamec);
		else
			priv->list_fname = g_strdup ("");
			
		g_object_notify (object, pspec->name);
		break;
		
	case CHANGED_PROP:
		priv->changed = g_value_get_boolean (value);
		g_object_notify (object, pspec->name);
		break;
		
	default:
	        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
	        break;
	}
}

static void
xfce_image_list_box_class_init(XfceImageListBoxClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	parent_class = g_type_class_peek_parent(klass);
	
	object_class->finalize = xfce_image_list_box_finalize;
	object_class->dispose = xfce_image_list_box_dispose;
	object_class->get_property = xfce_image_list_box_get_property;
	object_class->set_property = xfce_image_list_box_set_property;

	g_object_class_install_property (object_class, CHANGED_PROP,
		g_param_spec_boolean ("changed", "If Changed",
			"If the image list has been changed",
			FALSE,
			G_PARAM_READABLE|G_PARAM_WRITABLE
		)
	);

	g_object_class_install_property (object_class, FNAME_PROP,
		g_param_spec_string ("filename", "Filename of List",
			"The file name of the image list file",
			NULL,
			G_PARAM_READABLE|G_PARAM_WRITABLE
		)
	);
	
	XfceImageListBox_signals[SELECTION_CHANGED_SIGNAL] = g_signal_new ("selection-changed", 
		TYPE_XFCE_IMAGE_LIST_BOX, 
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (XfceImageListBoxClass, selection_changed),
		NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0
	);
		
}

static void
_my_tree_path_free_cb(GtkTreePath *pa, gpointer user_data)
{
	gtk_tree_path_free (pa);
}

static void
xfce_image_list_load_detail_if_needed (XfceImageListBox *il, GtkTreePath *tpath)
{
	gchar *fpath;
	GtkTreeIter iter;
	gboolean loaded_detail;
	XfceImageListBoxPrivate *priv;
	priv = il->priv;
	
	fpath = NULL;
	if (gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->store), &iter, tpath)) {
		loaded_detail = TRUE;
		gtk_tree_model_get (GTK_TREE_MODEL (priv->store), &iter, 
				C_LOADED_DETAIL, &loaded_detail, 
				C_PATH, &fpath,
			-1);
		} else {
			loaded_detail = TRUE;
		}
			
		if (!loaded_detail) {
			TRACE ("loading detail");
			loaded_detail = TRUE;
			gtk_list_store_set (priv->store, &iter, C_LOADED_DETAIL, loaded_detail, -1);
			g_object_set (priv->thumb_renderer, 
				"iter", &iter,
				"path", fpath,
				"pixbuf", NULL,
			 	NULL
			 );
			xfce_cell_renderer_pixbuf_on_demand_load (priv->thumb_renderer, GDK_INTERP_BILINEAR);
		}
			
		if (fpath)
			g_free (fpath);
}

static void
xfce_image_list_box_selection_changed_cb(GtkTreeSelection *sels,
	XfceImageListBox *a
)
{
	XfceImageListBoxPrivate *priv;
	GtkTreePath *tpath;
	GList *gg;
	GList *g;
	
	priv = a->priv;
	
	tpath = NULL;
	gtk_tree_view_get_cursor (priv->tree, &tpath, NULL);
	if (tpath) {
		xfce_image_list_load_detail_if_needed (a, tpath);
		gtk_tree_path_free (tpath);
	}

#if 0
	gg = gtk_tree_selection_get_selected_rows (sels, NULL);
	if (gg) {
		/*g_object_get (priv->thumb_renderer, "iter", &old_iter, NULL);*/
	
		g = gg;
		while (g) {
			tpath = (GtkTreePath *)g->data;
			xfce_image_list_load_detail_if_needed (a, tpath);
			if (tpath)
				gtk_tree_path_free (tpath);
				
			g = g_list_next (g);
		}
	
		/*g_object_set (priv->thumb_renderer, "iter", &old_iter, NULL);*/
	}
#endif
		
	g_signal_emit (G_OBJECT (a), XfceImageListBox_signals[SELECTION_CHANGED_SIGNAL], 0);
}

/*static void
xfce_image_list_box_add_folder_clicked_cb(GtkButton *b, XfceFileChooser *fc)
{
	gtk_dialog_response (GTK_DIALOG (fc), GTK_RESPONSE_NO);
}*/

static void
fnames_slist_item_free_cb(gpointer data, gpointer user_data)
{
	g_free (data);
}

static void
xfce_image_list_box_add_clicked_cb(GtkButton *b, XfceImageListBox *a)
{
	XfceImageListBoxPrivate *priv;
	XfceFileChooser *fc;
	XfceFileFilter *filt1;
	GSList *fnames, *cfn;
	gchar const *fname;
	int rc;
	/*GtkButton *add_folder_button;*/

	priv = a->priv;
	fc = XFCE_FILE_CHOOSER (xfce_file_chooser_new ("Background List - Add",
		GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (a))),
		XFCE_FILE_CHOOSER_ACTION_OPEN/*|XFCE_FILE_CHOOSER_ACTION_SELECT_FOLDER*/,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		_("_Add Folder"), GTK_RESPONSE_NO,
		_("_Add File"), GTK_RESPONSE_OK,
		NULL
		));

	/*add_folder_button = GTK_BUTTON (gtk_button_new_with_label (_("Add _Folder")));
	gtk_widget_show (GTK_WIDGET (add_folder_button));*/
	/*xfce_file_chooser_set_extra_widget (fc, GTK_WIDGET (add_folder_button));*/
	/*g_signal_connect (G_OBJECT (add_folder_button), "clicked", 
		G_CALLBACK(xfce_image_list_box_add_folder_clicked_cb),
		fc);*/
		
	xfce_file_chooser_set_select_multiple (fc, TRUE);
	/*set_current_folder*/
	/*set_preview_widget*/
	/*set_use_preview_label*/

	filt1 = xfce_file_filter_new ();
	xfce_file_filter_set_name (filt1, _("Images"));
	xfce_file_filter_add_mime_type (filt1, "image/*");
	xfce_file_filter_add_pattern (filt1, "*.png");
	xfce_file_filter_add_pattern (filt1, "*.jpg");
	xfce_file_filter_add_pattern (filt1, "*.gif");
	xfce_file_filter_add_pattern (filt1, "*.bmp");
	xfce_file_filter_add_pattern (filt1, "*.png");
	xfce_file_filter_add_pattern (filt1, "*.pnm");
	xfce_file_filter_add_pattern (filt1, "*.xpm");
	xfce_file_filter_add_pattern (filt1, "*.tif");
	
	xfce_file_chooser_add_filter (fc, filt1);
	
	rc = gtk_dialog_run (GTK_DIALOG (fc));
	if (rc == GTK_RESPONSE_OK || rc == GTK_RESPONSE_NO) {
		fnames = xfce_file_chooser_get_filenames (fc);
		if(!fnames)
			return;
			
		cfn = fnames;
		a->priv->check_files++;
		while (cfn) {
			fname = cfn->data;
			if (rc == GTK_RESPONSE_OK) {
				xfce_image_list_box_add_file (a, fname);
			} else if (rc == GTK_RESPONSE_NO) {
				xfce_image_list_box_add_directory (a, fname, 0);
			}
			
			cfn = g_slist_next (cfn);
		}
		a->priv->check_files--;
		
		g_slist_foreach (fnames, fnames_slist_item_free_cb, NULL);
		g_slist_free (fnames);
	}
	
	gtk_widget_destroy (GTK_WIDGET (fc));
}

void 
xfce_image_list_box_remove (XfceImageListBox *a, GtkTreeIter *iter)
{
	gchar *filename;
	filename = NULL;
	gtk_tree_model_get (GTK_TREE_MODEL (a->priv->store), iter,
		C_PATH, &filename, -1);
		
	gtk_list_store_remove (GTK_LIST_STORE (a->priv->store), iter);

	if (filename) {	
		g_hash_table_remove (a->priv->existing_paths, filename);
		g_free (filename);
	}
	/*g_object_set (G_OBJECT (aXfceImageListBox->priv->thumb_renderer), "iter", NULL, NULL);*/

}
static void delp (GtkTreePath *tpath, XfceImageListBox *aXfceImageListBox)
{
	GtkTreeIter iter;
	gtk_tree_model_get_iter (GTK_TREE_MODEL (aXfceImageListBox->priv->store), &iter, tpath);
	
	xfce_image_list_box_remove (aXfceImageListBox, &iter);
	
}

static void
xfce_image_list_box_set_changed(XfceImageListBox *a, gboolean changed);

static void
xfce_image_list_box_remove_clicked_cb(GtkButton *b, XfceImageListBox *aXfceImageListBox)
{
	GtkTreeSelection *sels;
	GList *gg, *g;
	gboolean chg;
	sels = gtk_tree_view_get_selection (aXfceImageListBox->priv->tree);
	
	gg = gtk_tree_selection_get_selected_rows (sels, NULL);
	if (gg) {
		chg  = FALSE;
		gg = g_list_reverse (gg);
		
		chg = g_list_length (gg) > 0;
		

		TRACE ("listbox:remove_clicked: before delp");
		g_list_foreach (gg, (GFunc)delp, aXfceImageListBox);
		TRACE ("listbox:remove_clicked: after delp");
		
		g_list_foreach (gg, (GFunc) _my_tree_path_free_cb, NULL);
		g_list_free (gg);
		
		
		xfce_image_list_box_set_changed (aXfceImageListBox, TRUE);
	}
}

static void
xfce_image_list_box_move_selection (XfceImageListBox *a, GdkScrollDirection dir)
{
	GtkTreePath *curpath; /* cursor path */
	gboolean movecursor; /* need to move cursor away afterwards ? */
	GtkTreeSelection *sels;
	GtkTreeModel *model;
	GList *paths; /* the selected rows as tree paths */
	GList *nselps; /* the newly selected rows as tree paths */
	GList *nselp1; /* one item in nselps */
	GList *path1; /* one item in paths */
	GtkTreePath *path; /* current path */
	GtkTreePath *ppath; /* previous (or next) path */
	GtkTreeIter iter; /* iter to path */
	GtkTreeIter piter; /* iter to ppath */
	gboolean changed;
	
	if (dir != GDK_SCROLL_UP && dir != GDK_SCROLL_DOWN)
		return;
	
	changed = FALSE;

	curpath = NULL;
	gtk_tree_view_get_cursor (a->priv->tree, &curpath, NULL);
	
	sels = gtk_tree_view_get_selection (a->priv->tree);
	paths = gtk_tree_selection_get_selected_rows (sels, &model);
	nselps = NULL;
	
	model = GTK_TREE_MODEL (a->priv->store);

	if (dir == GDK_SCROLL_DOWN)
		paths = g_list_reverse (paths);

	path1 = paths;
	while (path1) {
		path = (GtkTreePath *) (path1->data);
		if (gtk_tree_path_compare (curpath, path) == 0)
			movecursor = TRUE;

		if (dir == GDK_SCROLL_DOWN)
			ppath = tree_path_dup_and_next (path);
		else
			ppath = tree_path_dup_and_prev (path);
			
		if (gtk_tree_model_get_iter (model, &iter, path)
		&& ppath
		&& gtk_tree_model_get_iter (model, &piter, ppath)
		) {
			nselps = g_list_append (nselps, gtk_tree_path_copy (ppath));
			changed = TRUE;
			listm_swap (GTK_LIST_STORE (model), &iter, &piter);
		}
		
		if (ppath) {
			gtk_tree_path_free (ppath);	
			ppath = NULL;
		}
		path1 = g_list_next (path1);
	}
	
	if (movecursor && curpath) {
		if (dir == GDK_SCROLL_UP)
			path = tree_path_dup_and_prev (curpath);
		else
			path = tree_path_dup_and_next (curpath);
			
		if (path) {
			gtk_tree_view_set_cursor (a->priv->tree, path, NULL, FALSE);
			if (dir == GDK_SCROLL_UP) 
				gtk_tree_view_scroll_to_cell (a->priv->tree, path, NULL, TRUE, 0, 0);
			else if (dir == GDK_SCROLL_DOWN)
				gtk_tree_view_scroll_to_cell (a->priv->tree, path, NULL, TRUE, 1, 0);
				
			gtk_tree_path_free (path);
		}
	}
	
	gtk_tree_selection_unselect_all (sels);
	
	nselp1 = nselps;
	while (nselp1) {
		gtk_tree_selection_select_path (sels, 
			(GtkTreePath *) (nselp1->data)
		);
		nselp1 = g_list_next (nselp1);
	}
	

	if (nselps) {
		if (g_list_length (nselps) == 0)
			gtk_tree_selection_select_path (sels, curpath);
		
		g_list_foreach (nselps, (GFunc) _my_tree_path_free_cb, NULL);
		g_list_free (nselps);
		nselps = NULL;
	}
	if (paths) {
		g_list_foreach (paths, (GFunc) _my_tree_path_free_cb, NULL);
		g_list_free (paths);
		paths = NULL;
	}
	if (curpath) {
		gtk_tree_path_free (curpath);
		curpath = NULL;
	}
	
	if (changed)
		xfce_image_list_box_set_changed (a, TRUE);
}

static void
xfce_image_list_box_up_clicked_cb(GtkButton *b, XfceImageListBox *aXfceImageListBox)
{
	xfce_image_list_box_move_selection (aXfceImageListBox, GDK_SCROLL_UP);
}

static void
xfce_image_list_box_down_clicked_cb(GtkButton *b, XfceImageListBox *aXfceImageListBox)
{
	xfce_image_list_box_move_selection (aXfceImageListBox, GDK_SCROLL_DOWN);
}

static void
xfce_image_list_box_insert_file(XfceImageListBox *a, const gchar *filename, GtkTreeIter *before);

#if 0
static gchar *
urlunescape(gchar const *s)
{
	gchar **l;
	gchar *t;
	gchar *xhostname;
	
	if (!g_str_has_prefix(s, "file://"))
		return g_strdup (s);

	t = g_filename_from_uri (s, &xhostname, NULL);
	/* TODO check hostname */
	g_free (xhostname);
	return t;
}
#endif

static void
xfce_image_list_box_insert_stuff(XfceImageListBox *a, const gchar *filename, GtkTreeIter *before, int loopy);

static lb_data_get_free_cb(GtkTreePath *tpath, XfceImageListBox *a)
{
	gchar *fpath;
	GtkTreeIter iter;
	if (gtk_tree_model_get_iter (GTK_TREE_MODEL (a->priv->store), &iter, tpath)) {
		gtk_tree_model_get (GTK_TREE_MODEL (a->priv->store), &iter, C_PATH, &fpath, -1);
		
		if (fpath) {
			a->priv->txtl = g_list_append (a->priv->txtl, g_strdup (fpath));
			g_free (fpath);
		}
	}
	gtk_tree_path_free (tpath);
}
 
static void
xfce_image_list_box_data_get_cb(GtkWidget *w, GdkDragContext *context, 
	GtkSelectionData *data, guint info, guint time, XfceImageListBox *a)
{
	gchar *txts;
	gchar *txtsn;
	gchar *txt;
	GList *selsl;
	GList *g;
	GtkTreeSelection *sels;
	/* info: format */
	/*gtk_selection_data_set (data, type, format, data, length); */
	
	txts = NULL;
	
	sels = gtk_tree_view_get_selection (GTK_TREE_VIEW (w));
	if (sels) {
		selsl = gtk_tree_selection_get_selected_rows (sels, NULL);
		
		a->priv->txtl = NULL;
		g_list_foreach (selsl, (GFunc)lb_data_get_free_cb, a);
		
		
		g_list_free(selsl);

		g = a->priv->txtl;
		while (g) {
			txt = ((gchar *)g->data);
			
			if (txts) {
				txtsn = g_strdup_printf("%s%s\n", txts, txt);
				g_free (txts);
				txts = txtsn;
			} else
				txts = g_strdup (txt);
			
			g_free (txt);
			g = g_list_next (g);
		}
		
		g_list_free (a->priv->txtl);
	}
	
	if (txts)  {
		gtk_selection_data_set_text (data, txts, -1);
		/*gtk_selection_data_set(data, 8, txts);*/
	}
}

static gboolean
xfce_image_list_box_check_file_is_image(XfceImageListBox *lb, gchar const *filename)
{
	if (!filename)
		return FALSE;
		
	if (lb->priv->check_files <= 0)
		return TRUE;
		
	#ifdef TEST_IMAGE_FILES_BEFORE_ADD
	return (
		g_file_test (filename, G_FILE_TEST_IS_REGULAR)
	||
		gdk_pixbuf_get_file_info(filename, NULL, NULL)
	);
	#endif
	
	return TRUE;
}

static void
xfce_image_list_box_data_received_cb(GtkWidget *w, GdkDragContext *context, gint x, gint y,
	GtkSelectionData *data, guint info, guint time, XfceImageListBox *a)
{
	XfceImageListBoxPrivate *priv;
	GtkTreePath *tpath;
	GtkTreeIter iter;
	GtkTreeIter *niter;
	guchar *xdatas;
	gchar **xdata;
	gchar *s;
	gchar *t;
	int i;
	
	priv = a->priv;
	
	tpath = NULL;
	gtk_tree_view_get_drag_dest_row (priv->tree, &tpath, NULL);
	
	niter = NULL;
	if (tpath) {
		if (gtk_tree_model_get_iter (GTK_TREE_MODEL(priv->store), &iter, tpath))
			niter = &iter;
		
		gtk_tree_path_free (tpath);
	}
	
	xdatas = data->data; /*gtk_selection_data_get_text (data);*/
	if (xdatas)
		xdata = g_strsplit ((gchar*) xdatas, "\n", 0);
	else
		xdata = NULL;
		
	if (xdata) {
		priv->check_files++;
		for(i = 0; xdata[i]; i++) {
			s = xdata[i];
			if (g_str_has_suffix (s, "\r"))
				s[strlen(s) - 1] = 0;
			if (g_str_has_suffix (s, "\n"))
				s[strlen(s) - 1] = 0;
			if (g_str_has_suffix (s, "\r"))
				s[strlen(s) - 1] = 0;
			
			if (s && s[0]) {
				if (g_str_has_prefix(s, "file://")) {
					t = g_filename_from_uri (s, NULL, NULL);
				} else t = s;
				if (t && t[0]) {
					xfce_image_list_box_insert_stuff (a, t, niter, 0);
				}
					
				if (t && t != s) {
					g_free (t);
					t = NULL;
				}
			}
		}
		priv->check_files--;
		g_strfreev (xdata);
	}
	
}


static void 
xfce_image_list_box_init_tree_columns(XfceImageListBox *a);

static void 
xfce_image_list_box_init(XfceImageListBox *aXfceImageListBox)
{
	XfceImageListBoxPrivate *priv;
	GtkTreeSelection *sel;

	priv = g_new0(XfceImageListBoxPrivate, 1);
	
	priv->existing_paths = g_hash_table_new_full (
		g_str_hash,
		g_str_equal,
		g_free,
		(GDestroyNotify)gtk_tree_iter_free
	);
	
	priv->thumb_width = 128;
	priv->thumb_height = 128; /* TODO setting */

	aXfceImageListBox->priv = priv;
		
	priv->tree = GTK_TREE_VIEW (gtk_tree_view_new ()); /* list */
	gtk_tree_view_set_reorderable (priv->tree, TRUE);

	priv->store = GTK_LIST_STORE (gtk_list_store_new (5,
		G_TYPE_OBJECT,
		G_TYPE_STRING,
		G_TYPE_STRING,
		G_TYPE_BOOLEAN,
		G_TYPE_BOOLEAN
	));

	xfce_image_list_box_init_tree_columns (aXfceImageListBox);
	

	priv->vbuttons = GTK_VBUTTON_BOX (gtk_vbutton_box_new ());
	priv->ok_button = NULL;
	
	sel = gtk_tree_view_get_selection (priv->tree);
	gtk_tree_selection_set_mode (sel, GTK_SELECTION_MULTIPLE);
	
	gtk_tree_view_set_headers_visible (priv->tree, TRUE);
	
	g_signal_connect (G_OBJECT (priv->tree), "drag-data-received", 
		G_CALLBACK(xfce_image_list_box_data_received_cb), aXfceImageListBox);
	g_signal_connect (G_OBJECT (priv->tree), "drag-data-get", 
		G_CALLBACK(xfce_image_list_box_data_get_cb), aXfceImageListBox);
	gtk_tree_view_enable_model_drag_dest (priv->tree, targets, G_N_ELEMENTS (targets), GDK_ACTION_DEFAULT);
	
	gtk_tree_view_enable_model_drag_source (priv->tree, 
		/*GDK_SHIFT_MASK|GDK_CONTROL_MASK|*/GDK_BUTTON1_MASK|GDK_BUTTON3_MASK
		/*GDK_BUTTON2_MASK*/,
		targets, G_N_ELEMENTS (targets), GDK_ACTION_COPY
	);

	if (1) {
		gtk_tree_view_column_set_fixed_width (priv->thumb_column, priv->thumb_width + 5);
		gtk_tree_view_column_set_sizing (priv->thumb_column, GTK_TREE_VIEW_COLUMN_FIXED);
		/*col1 likewise*/
		/*tree.set_property("fixed-height-mode", TRUE);*/
	}	

	gtk_tree_view_set_model (priv->tree, GTK_TREE_MODEL (priv->store));
	g_signal_connect (G_OBJECT (sel), "changed", G_CALLBACK(xfce_image_list_box_selection_changed_cb), aXfceImageListBox);
	
	priv->ag = GTK_ACCEL_GROUP (gtk_accel_group_new ());

	/* TODO -> actions */	
	priv->add_button = GTK_BUTTON (gtk_button_new_with_mnemonic ("_Add ..."));
	gtk_widget_add_accelerator (GTK_WIDGET (priv->add_button), "clicked", priv->ag, gdk_keyval_from_name("Insert"), 0, GTK_ACCEL_VISIBLE);
	g_signal_connect (G_OBJECT (priv->add_button), "clicked", 
		G_CALLBACK(xfce_image_list_box_add_clicked_cb), aXfceImageListBox);

	priv->remove_button = GTK_BUTTON (gtk_button_new_with_mnemonic ("_Remove"));
	gtk_widget_add_accelerator (GTK_WIDGET (priv->remove_button), "clicked", priv->ag, gdk_keyval_from_name("Delete"), 0, GTK_ACCEL_VISIBLE);
	g_signal_connect (G_OBJECT (priv->remove_button), "clicked", 
		G_CALLBACK(xfce_image_list_box_remove_clicked_cb), aXfceImageListBox);

	priv->up_button = GTK_BUTTON (gtk_button_new_with_mnemonic ("_Up"));
	gtk_widget_add_accelerator (GTK_WIDGET (priv->up_button), "clicked", priv->ag, gdk_keyval_from_name("Up"), 0, GTK_ACCEL_VISIBLE);
	g_signal_connect (G_OBJECT (priv->up_button), "clicked", 
		G_CALLBACK(xfce_image_list_box_up_clicked_cb), aXfceImageListBox);

	priv->down_button = GTK_BUTTON (gtk_button_new_with_mnemonic ("_Down"));
	gtk_widget_add_accelerator (GTK_WIDGET (priv->down_button), "clicked", priv->ag, gdk_keyval_from_name("Down"), 0, GTK_ACCEL_VISIBLE);
	g_signal_connect (G_OBJECT (priv->down_button), "clicked", 
		G_CALLBACK(xfce_image_list_box_down_clicked_cb), aXfceImageListBox);

	/*gtk_window_add_accel_group(priv->ag);*/
	
	/* buttons */
	gtk_box_set_spacing (GTK_BOX (priv->vbuttons), 7);
	gtk_container_set_border_width (GTK_CONTAINER (priv->vbuttons), 7);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (priv->vbuttons), GTK_BUTTONBOX_START);
	
	gtk_box_pack_start (GTK_BOX (priv->vbuttons), GTK_WIDGET (priv->add_button), FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (priv->vbuttons), GTK_WIDGET (priv->remove_button), FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (priv->vbuttons), GTK_WIDGET (priv->up_button), FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (priv->vbuttons), GTK_WIDGET (priv->down_button), FALSE, FALSE, 0);

	gtk_widget_show_all (GTK_WIDGET (priv->vbuttons));

	/* layout containers */
	
	priv->scroller = GTK_SCROLLED_WINDOW (gtk_scrolled_window_new (NULL, NULL));
	gtk_scrolled_window_set_policy (priv->scroller, GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (priv->scroller), GTK_WIDGET(priv->tree));
	gtk_widget_show_all (GTK_WIDGET (priv->scroller));

	priv->left_box = GTK_BOX (gtk_hbox_new (FALSE, 7));
	gtk_box_pack_start (priv->left_box, GTK_WIDGET (priv->scroller), TRUE, TRUE, 0);
	gtk_box_pack_start (priv->left_box, GTK_WIDGET (priv->vbuttons), FALSE, FALSE, 0);
	gtk_widget_show (GTK_WIDGET (priv->left_box));
	
	gtk_box_pack_start (GTK_BOX (aXfceImageListBox), GTK_WIDGET (priv->left_box), TRUE, TRUE, 0);

}

XfceImageListBox*
xfce_image_list_box_new(void)
{
	XfceImageListBox *aXfceImageListBox;
	XfceImageListBoxPrivate *priv;

	aXfceImageListBox = g_object_new(xfce_image_list_box_get_type(), NULL);
	
	priv = aXfceImageListBox->priv;

	return aXfceImageListBox;
}


static void
xfce_image_list_box_set_changed(XfceImageListBox *a, gboolean changed)
{
	XfceImageListBoxPrivate *priv;
	priv = a->priv;
	if (priv->changed != changed) {
		TRACE ("listbox: set changed %s", changed ? "true" : "false" );
		g_object_set (G_OBJECT (a), "changed", changed, NULL);
	}
}

/*
up_clicked_cb
down_clicked_cb
*/

static void
xfce_image_list_box_data_func_cb(GtkTreeViewColumn *tree_column,
	GtkCellRenderer *cell,
	GtkTreeModel *tree_model,
	GtkTreeIter *iter,
	gpointer data
)
{
	g_object_set (G_OBJECT (cell), "iter", iter, NULL);
/* TODO */
}

/**/
static void 
xfce_image_list_box_init_tree_columns(XfceImageListBox *a)
{
	XfceCellRendererPixbufOnDemand *cell0;
	GtkCellRendererText *cell1;
	XfceImageListBoxPrivate *priv;
	GtkTreeViewColumn *col0;
	GtkTreeViewColumn *col1;

	priv = a->priv;

	cell0 = xfce_cell_renderer_pixbuf_on_demand_new ();

	xfce_cell_renderer_pixbuf_on_demand_set_size (cell0, priv->thumb_width, priv->thumb_height);

	col0 = gtk_tree_view_column_new_with_attributes (
		"", GTK_CELL_RENDERER (cell0),
		"pixbuf", C_THUMB,
		"path", C_PATH,
		"loaded", C_LOADED,
		NULL
	);
	
	xfce_cell_renderer_pixbuf_on_demand_set_model (cell0, GTK_LIST_STORE (priv->store));
	xfce_cell_renderer_pixbuf_on_demand_set_loaded_column (cell0, C_LOADED);
	xfce_cell_renderer_pixbuf_on_demand_set_thumb_column (cell0, C_THUMB);
	
	gtk_tree_view_column_set_cell_data_func (col0, 
		GTK_CELL_RENDERER (cell0), 
		xfce_image_list_box_data_func_cb, NULL, NULL
	);

	gtk_tree_view_append_column (priv->tree, col0);

	priv->thumb_column = col0;
	priv->thumb_renderer = cell0;
	
	cell1 = GTK_CELL_RENDERER_TEXT (gtk_cell_renderer_text_new ());
	col1 = gtk_tree_view_column_new_with_attributes (
		"Name", GTK_CELL_RENDERER (cell1),
		"text", C_NAME,
		NULL
	);
	
	gtk_tree_view_append_column (priv->tree, col1);
	
/*	#define C_THUMB 0
	#define C_NAME 1
	#define C_PATH 2
	#define C_LOADED 3
	#define C_ITER 4
*/	
}

static void 
xfce_image_list_box_init_pointers(XfceImageListBox *a)
{
}

static void
xfce_image_list_box_set_wait_cursor(XfceImageListBox *a, gboolean wait)
{
/*	XfceImageListBoxPrivate *priv;
	priv = a->priv;
*/
}

static void
xfce_image_list_box_insert_file(XfceImageListBox *a, const gchar *filename, GtkTreeIter *before);

static void
xfce_image_list_box_insert_directory(XfceImageListBox *a, const gchar *dirname, GtkTreeIter *before, int loopy)
{
	GDir *gdir;
	gchar const *s;
	gchar *spath;
	gdir = g_dir_open (dirname, 0, NULL);
	if (gdir) {
		while ((s = g_dir_read_name (gdir)) != NULL) {
			spath = g_build_filename (dirname, s, NULL);
			if (spath) {
				if (loopy < MAX_DIR_DEPTH)
					xfce_image_list_box_insert_stuff (a, spath, before, loopy + 1);
					
				g_free (spath);
			}
		}
		
		g_dir_close (gdir);
	}
}

static gchar *
shortname(gchar *disposethis)
{
	gchar *s;
	if (!disposethis)
		return NULL;
		
	if (g_utf8_strlen (disposethis, -1) > MAX_TITLE_LEN) {
		s = g_strdup (disposethis);
		g_utf8_strncpy (s, disposethis, MAX_TITLE_LEN);
		g_free (disposethis);
		return s;
	}
	return disposethis;
}

static void
xfce_image_list_box_insert_file(XfceImageListBox *a, const gchar *filename, GtkTreeIter *before)
{
	XfceImageListBoxPrivate *priv;
	gchar *n;
	gchar *os;
	GtkTreeIter iter;
	
	priv = a->priv;
	
	if (!xfce_image_list_box_check_file_is_image(a, filename))
		return;
		
	/* check for dupes */
	if (g_hash_table_lookup (priv->existing_paths, filename))
		return;

	if(before)
		gtk_list_store_insert_before (priv->store, &iter, before);
	else
		gtk_list_store_append (priv->store, &iter);
	
	os = g_path_get_basename (filename);
	n = shortname (g_filename_to_utf8 (os, -1, NULL, NULL, NULL));
	if (os)
		g_free (os);
	
	
	/* C_THUMB */
	gtk_list_store_set (priv->store, &iter,
		C_NAME, n,
		C_PATH, filename,
		C_LOADED, FALSE,
		C_LOADED_DETAIL, FALSE,
		-1
	);

	if (n)	
		g_free (n);
	
	g_hash_table_insert (priv->existing_paths, g_strdup (filename), 
		gtk_tree_iter_copy (&iter)
	);
	
	xfce_image_list_box_set_changed (a, TRUE);
}

void
xfce_image_list_box_add_file(XfceImageListBox *a, const gchar *filename)
{
	xfce_image_list_box_insert_file(a, filename, NULL);
}

void
xfce_image_list_box_add_directory(XfceImageListBox *a, const gchar *filename, int loopy)
{
	xfce_image_list_box_insert_directory(a, filename, NULL, loopy);
}


void
xfce_image_list_box_add_stuff(XfceImageListBox *a, const gchar *filename, int loopy)
{
	xfce_image_list_box_insert_stuff(a, filename, NULL, loopy);
}

static void
xfce_image_list_box_insert_stuff(XfceImageListBox *a, const gchar *filename, GtkTreeIter *before, int loopy)
{
	if (g_file_test (filename, G_FILE_TEST_IS_DIR))
		xfce_image_list_box_insert_directory (a, filename, before, loopy);
	else
		xfce_image_list_box_insert_file(a, filename, before);
}



gboolean
xfce_image_list_box_load (XfceImageListBox *a, const gchar *filename)
{
	/* TODO error dialogs */
	FILE *f;
	char s[4096];
	XfceImageListBoxPrivate *priv;
	priv = a->priv;
	
	xfce_image_list_box_clear (a);
	
	g_assert (filename);

	f = fopen (filename, "r");
	if (f) {
		while (fgets(s, sizeof(s), f) != NULL) {
			while (s && s[0] && (g_str_has_suffix (s, "\n") || g_str_has_suffix (s, "\r"))) {
				s[strlen(s)-1] = 0;
			}			
			
			if (s && s[0] == '#') {
				/* skip comment */
			} else {
				xfce_image_list_box_add_stuff(a, s, 0);
			}
		}
		fclose (f);
		g_object_set (G_OBJECT (a), "filename", filename, NULL);
		xfce_image_list_box_set_changed (a, FALSE);
		return TRUE;
	}
	xfce_image_list_box_set_changed (a, FALSE); /* ? */
	return FALSE;
}

gboolean
xfce_image_list_box_save(XfceImageListBox *a, const gchar *filename)
{
	XfceImageListBoxPrivate *priv;
	gchar *fpath;
	FILE *f;
	GtkTreeIter iter;
	
	priv = a->priv;

	f = fopen(filename, "w");
	if (!f) {
		/* TODO error message */
		return FALSE;
	}
		
	fprintf(f, "# xfce backdrop list");
	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->store), &iter)) {
		do {
			gtk_tree_model_get (GTK_TREE_MODEL (priv->store), &iter, C_PATH, &fpath, -1);
			if (fpath) {	
				fprintf(f, "\n%s", fpath);
				g_free (fpath);
			}
		} while (gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->store), &iter));
	}
	fclose(f);
	return TRUE;
}

void
xfce_image_list_box_clear(XfceImageListBox *a)
{
	XfceImageListBoxPrivate *priv;
	priv = a->priv;
	gtk_list_store_clear (priv->store);	
	
	g_object_set (G_OBJECT (a), "filename", "", NULL);
}

gchar *
xfce_image_list_box_get_selected_filename(XfceImageListBox *a)
{
	XfceImageListBoxPrivate *priv;
	GtkTreeSelection *sels;
	GtkTreePath *tpath;
	GtkTreeViewColumn *tcolumn;
	GtkTreeIter iter;
	gchar *fname;
	priv = a->priv;
	
	/*sels = gtk_tree_view_get_selection (priv->tree);*/
	gtk_tree_view_get_cursor (priv->tree, &tpath, &tcolumn);
	
	if (tpath) {
		gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->store), &iter, tpath);
		gtk_tree_path_free (tpath);
		
		gtk_tree_model_get (GTK_TREE_MODEL (priv->store), &iter, C_PATH, &fname, -1);
		return fname;
	}
	return NULL;
}

gboolean
xfce_image_list_box_find_filename(XfceImageListBox *a, const gchar *filename, GtkTreeIter *oiter)
{
	XfceImageListBoxPrivate *priv;
	GtkTreeIter iter;
	gboolean found;
	gchar *xfilename;
	GtkTreeIter *fastiter;
	
	priv = a->priv;
	
	found = FALSE;
	
	if (!filename)
		return FALSE;

	if (!priv->existing_paths)
		return FALSE;
	
	fastiter = ((GtkTreeIter *)g_hash_table_lookup (priv->existing_paths, filename));
	if (fastiter) {
		iter = *fastiter;
		*oiter = iter;
		return TRUE;
	}
	
	if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->store), &iter))
		return FALSE;
			
	do {
		xfilename = NULL;
		gtk_tree_model_get (GTK_TREE_MODEL (priv->store), 
			&iter, C_PATH, &xfilename, -1);
		
		if (g_str_equal (xfilename, filename)) {
			*oiter = iter;
			found = TRUE;
			
			if (xfilename)
				g_free (xfilename);
				
			break;
		}
		
		if (xfilename)
			g_free (xfilename);
	} while (gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->store), &iter));
	
	return found;
}

void
xfce_image_list_box_select_filename(XfceImageListBox *a, const gchar *filename)
{
	GtkTreeIter iter;
	GtkTreeSelection *sels;
	GtkTreePath *tpath;
	XfceImageListBoxPrivate *priv;

	priv = a->priv;

	if (xfce_image_list_box_find_filename (a, filename, &iter)) {
		sels = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->tree));
		tpath = gtk_tree_model_get_path (GTK_TREE_MODEL (priv->store), &iter);
		gtk_tree_view_set_cursor (GTK_TREE_VIEW (priv->tree), tpath, NULL, FALSE);
		gtk_tree_selection_select_iter (sels, &iter);
			
		gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (priv->tree), 
			tpath, NULL, TRUE, 0.5, 0);
		gtk_tree_path_free (tpath);
	}
}

