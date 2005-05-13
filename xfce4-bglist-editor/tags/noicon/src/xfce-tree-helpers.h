#ifndef __XFCE_TREE_HELPERS_H
#define __XFCE_TREE_HELPERS_H
#include <gtk/gtk.h>

void listm_swap(GtkListStore *model, GtkTreeIter *iter1, GtkTreeIter *iter2);
GtkTreePath *tree_path_dup_and_prev(GtkTreePath *path);
GtkTreePath *tree_path_dup_and_next(GtkTreePath *path);

#endif
