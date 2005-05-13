#include <gtk/gtk.h>
#include "xfce-tree-helpers.h"
#include "trace.h"

#define SPEEDHACK

#ifdef SPEEDHACK
#define C_THUMB 0
#define C_NAME 1
#define C_PATH 2
#define C_LOADED 3
#define C_LOADED_DETAIL 4
#endif

void listm_swap(GtkListStore *model, GtkTreeIter *iter1, GtkTreeIter *iter2)
{
	gint cnt;
	gint i;
	GValue *v1;
	GValue *v2;
	
	if (!iter1 || !iter2)
		return;
		
	v1 = g_new0 (GValue, 1);
	v2 = g_new0 (GValue, 1);
	
	TRACE ("listm_swap %p %p %p", model, iter1, iter2);
	
	cnt = gtk_tree_model_get_n_columns (GTK_TREE_MODEL (model));
	TRACE ("cnt %d", cnt);
	
	for(i = 0; i < cnt; i++) {
#ifdef SPEEDHACK
		if (i == C_THUMB) {
			gtk_list_store_set (model, iter1, i, NULL, -1);
			gtk_list_store_set (model, iter2, i, NULL, -1);
			continue;
		}
					
		if (i == C_LOADED || i == C_LOADED_DETAIL) {
			gtk_list_store_set (model, iter1, i, FALSE, -1);
			gtk_list_store_set (model, iter2, i, FALSE, -1);
			continue;
		}
#endif
		
		gtk_tree_model_get_value (GTK_TREE_MODEL (model), iter1, i, v1);
		gtk_tree_model_get_value (GTK_TREE_MODEL (model), iter2, i, v2);

		gtk_list_store_set_value (model, iter2, i, v1);
		gtk_list_store_set_value (model, iter1, i, v2);
		
		g_value_unset (v1);
		g_value_unset (v2);
	}
	
	if (v1)
		g_free (v1);
	if (v2)
		g_free (v2);
}

GtkTreePath *tree_path_dup_and_prev(GtkTreePath *path)
{
	GtkTreePath *npath;
	if (!path)
		return NULL;

	npath = gtk_tree_path_copy (path);
	
	if (!gtk_tree_path_prev (npath))
		npath = NULL;
		
	return npath;
}

GtkTreePath *tree_path_dup_and_next(GtkTreePath *path)
{
	GtkTreePath *npath;
	if (!path)
		return NULL;
		
	npath = gtk_tree_path_copy (path);
	
	gtk_tree_path_next (npath);
		
	return npath;
}


