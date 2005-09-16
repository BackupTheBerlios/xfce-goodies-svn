/***************************************************************************
 *            callbacks.c
 *
 *  Thu Jul 15 06:01:04 2004
 *  Last Update: 18/03/2005
 *  Copyright  2004 - 2005  bountykiller
 *  Email: masse_nicolas@yahoo.fr
 ****************************************************************************/
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "callbacks.h"
#include "types.h"
#include "avoid_deprecation.h"

t_qck_launcher_opt_dlg *_dlg;
static GtkWidget  *_icon_window;

GtkWidget* create_icon_window();
void show_icon_window( GtkTreeView *treeview, GtkTreePath *arg1,
										GtkTreeViewColumn *arg2, gpointer user_data);
void btn_clicked(GtkButton *button, gpointer icon_id);
void on_spin_value_changed(GtkSpinButton *spinbutton, gpointer user_data);
void on_btn_new_clicked(GtkButton *button, gpointer user_data);
void on_btn_remove_clicked(GtkButton *button, gpointer user_data);
void on_btn_edit_clicked(GtkButton *button, gpointer user_data);
void on_btn_up_clicked(GtkButton *button, gpointer user_data);
void on_btn_down_clicked(GtkButton *button, gpointer user_data);
void cmd_changed(GtkCellRendererText *cellrenderertext, gchar *arg1, 
								gchar *arg2, gpointer user_data);
//void on_tree_reorder(GtkTreeModel *treemodel, GtkTreePath *arg1, GtkTreeIter *arg2,
//                                	gpointer arg3, gpointer user_data);
void  file_chooser_preview_img (FileChooser *chooser, gpointer user_data);


GtkWindow*
_gtk_widget_get_parent_gtk_window(GtkWidget* widget)	
{
	for( ; widget; widget = gtk_widget_get_parent(widget)) 
	{
		if ( GTK_IS_WINDOW(widget) )
			return ( GTK_WINDOW(widget) );
	};
	return NULL;
}

//Creation functions
//******************************************************************************
t_qck_launcher_opt_dlg* create_qck_launcher_dlg()
 {
  GtkAdjustment *adjust;
  _dlg = (t_qck_launcher_opt_dlg *) g_new0(t_qck_launcher_opt_dlg, 1);

  _dlg->vbox = gtk_vbox_new(FALSE, 0);
  gtk_widget_show (_dlg->vbox);

  _dlg->linebox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (_dlg->linebox);
  gtk_box_pack_start (GTK_BOX (_dlg->vbox), _dlg->linebox, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (_dlg->linebox), 5);

  _dlg->label = gtk_label_new_with_mnemonic(_("Lines: "));
  gtk_widget_show (_dlg->label);
  gtk_box_pack_start (GTK_BOX (_dlg->linebox), _dlg->label, FALSE, FALSE, 0);

  _dlg->spin1 = gtk_spin_button_new_with_range(1, 5, 1);
  gtk_widget_show (_dlg->spin1);
  gtk_box_pack_start (GTK_BOX (_dlg->linebox), _dlg->spin1, FALSE, FALSE, 0);
  
  _dlg->configbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (_dlg->configbox);
  gtk_box_pack_start (GTK_BOX (_dlg->vbox), _dlg->configbox, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (_dlg->configbox), 5);
  
  _dlg->scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (_dlg->scrolledwindow1);
  gtk_box_pack_start (GTK_BOX (_dlg->configbox), _dlg->scrolledwindow1, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (_dlg->scrolledwindow1), 5);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (_dlg->scrolledwindow1), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (_dlg->scrolledwindow1), GTK_SHADOW_ETCHED_OUT);

  _dlg->treeview1 = gtk_tree_view_new ();
  gtk_widget_show (_dlg->treeview1);
  gtk_container_add (GTK_CONTAINER (_dlg->scrolledwindow1), _dlg->treeview1);
  gtk_container_set_border_width (GTK_CONTAINER (_dlg->treeview1), 3);
  gtk_widget_set_size_request(_dlg->treeview1, 200, 25*_quicklauncher->nb_launcher);

  _dlg->vbuttonbox1 = gtk_vbutton_box_new ();
  gtk_widget_show (_dlg->vbuttonbox1);
  gtk_box_pack_start (GTK_BOX (_dlg->configbox), _dlg->vbuttonbox1, FALSE, TRUE, 5);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (_dlg->vbuttonbox1), GTK_BUTTONBOX_SPREAD);

  _dlg->btn_new = gtk_button_new_from_stock ("gtk-new");
  gtk_widget_show (_dlg->btn_new);
  gtk_container_add (GTK_CONTAINER (_dlg->vbuttonbox1), _dlg->btn_new);
  GTK_WIDGET_SET_FLAGS (_dlg->btn_new, GTK_CAN_DEFAULT);

  _dlg->btn_remove = gtk_button_new_from_stock ("gtk-delete");
  gtk_widget_show (_dlg->btn_remove);
  gtk_container_add (GTK_CONTAINER (_dlg->vbuttonbox1), _dlg->btn_remove);
  GTK_WIDGET_SET_FLAGS (_dlg->btn_remove, GTK_CAN_DEFAULT);
 
  _dlg->btn_up = gtk_button_new_from_stock ("gtk-go-up");
  gtk_widget_show (_dlg->btn_up);
  gtk_container_add (GTK_CONTAINER (_dlg->vbuttonbox1), _dlg->btn_up);
  GTK_WIDGET_SET_FLAGS (_dlg->btn_up, GTK_CAN_DEFAULT);

  _dlg->btn_down = gtk_button_new_from_stock ("gtk-go-down");
  gtk_widget_show (_dlg->btn_down);
  gtk_container_add (GTK_CONTAINER (_dlg->vbuttonbox1), _dlg->btn_down);
  GTK_WIDGET_SET_FLAGS (_dlg->btn_down, GTK_CAN_DEFAULT);
  
  return _dlg;	
}

/*
The prob here is that combo can't display a pixbuf so I use a window
-->keeping it since it can become usefull at a later date (not finalised at all)
void configure_combo(GtkCellRenderer *render)
{
	GtkTreeModel *treemodel;
	GdkPixbuf *pixbuf;
	gint i;
	treemodel  = GTK_TREE_MODEL(gtk_list_store_new(2, GDK_TYPE_PIXBUF, G_TYPE_STRING);
	for(i=0; i <XFCE_N_BUILTIN_ICON_CATEGORIES; ++i)
	{
		pixbuf = xfce_icon_theme_load_category(DEFAULT_ICON_THEME, i, 16);
		gtk_list_store_insert(GTK_LIST_STORE(treemodel), &iter, 0);
		gtk_list_store_set(GTK_LIST_STORE(treemodel), &iter, 0, pixbuf, 1, icons_categories_names[i], -1);
		if(pixbuf) 
			g_object_unref(pixbuf);
	}		
	g_object_set(G_OBJECT(render), "has-entry" , FALSE, "model", treemodel, "text-column", 1, NULL);
}*/


void
fill_qck_launcher_dlg()
{
	GtkTreeModel *treemodel;
	GList *i; 	
	GtkTreeIter iter;	
	GdkPixbuf *pixbuf;
	GtkTreeViewColumn *column;
	GtkCellRenderer *render;
	t_launcher *launcher;
	
	_icon_window = create_icon_window();
	treemodel  = GTK_TREE_MODEL(gtk_list_store_new(3, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_POINTER));
	gtk_tree_view_set_model(GTK_TREE_VIEW(_dlg->treeview1), treemodel);	
	gtk_tree_view_set_reorderable (GTK_TREE_VIEW (_dlg->treeview1), FALSE);//==>besoin de gérer le reorder
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(_dlg->spin1), (gdouble)_quicklauncher->nb_lines);
	
	render = gtk_cell_renderer_pixbuf_new();
	//render = gtk_cell_renderer_combo_new();
	//configure_combo(render); if I one day can use combo with pixbuf... 
	g_object_set (G_OBJECT(render), "mode", GTK_CELL_RENDERER_MODE_ACTIVATABLE,"sensitive", TRUE, NULL);
	column = gtk_tree_view_column_new_with_attributes(_("icone"), render, "pixbuf", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(_dlg->treeview1), column);
		
	render = gtk_cell_renderer_text_new();
	g_object_set (G_OBJECT(render),"editable", TRUE, NULL);
	g_signal_connect(render, "edited", G_CALLBACK (cmd_changed), NULL);
	column = gtk_tree_view_column_new_with_attributes(_("commande"), render, "text", 1,  NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(_dlg->treeview1), column);

	//load current config
	for( i = g_list_last(_quicklauncher->launchers); i != NULL; i = g_list_previous(i) )
	{
		launcher = i->data;
		gtk_list_store_insert(GTK_LIST_STORE(treemodel), &iter, 0);
		pixbuf = _create_pixbuf(launcher->icon_id, launcher->icon_name, 16);
		gtk_list_store_set(GTK_LIST_STORE(treemodel), &iter, 0, pixbuf, 
									 1,  launcher->command, 2, (gpointer)launcher, -1);
		UNREF(pixbuf);
	}	
	g_signal_connect(_dlg->treeview1, "row-activated", 
								G_CALLBACK(show_icon_window), NULL);
	g_signal_connect((gpointer)_dlg->spin1, "value-changed",
								G_CALLBACK (on_spin_value_changed), NULL);
	g_signal_connect ((gpointer) _dlg->btn_new, "clicked",
								G_CALLBACK (on_btn_new_clicked), NULL);
	g_signal_connect ((gpointer) _dlg->btn_remove, "clicked",
								G_CALLBACK (on_btn_remove_clicked),  NULL);
	g_signal_connect ((gpointer) _dlg->btn_up, "clicked",
								G_CALLBACK (on_btn_up_clicked), NULL);
	g_signal_connect ((gpointer) _dlg->btn_down, "clicked",
								G_CALLBACK (on_btn_down_clicked), NULL);
}


void
free_qck_launcher_dlg(GtkButton *button, gpointer user_data)
{
	gtk_widget_destroy(_icon_window);
}


GtkWidget* create_icon_window()
{
	GtkWidget *hbox, *btn;
	GdkPixbuf *pixbuf;
	gint i;
	
	_icon_window = gtk_window_new(GTK_WINDOW_POPUP);
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(_icon_window), hbox);
	for(i=0; i <XFCE_N_BUILTIN_ICON_CATEGORIES; ++i)
	{
		pixbuf = xfce_icon_theme_load_category(DEFAULT_ICON_THEME, i, 16);
		btn = xfce_iconbutton_new();
		gtk_button_set_relief (GTK_BUTTON (btn), GTK_RELIEF_NONE);
		xfce_iconbutton_set_pixbuf(XFCE_ICONBUTTON (btn), pixbuf);
		UNREF(pixbuf);
		gtk_box_pack_start(GTK_BOX(hbox), btn, TRUE, TRUE, 1);
		g_signal_connect(btn, "clicked", G_CALLBACK(btn_clicked), (gpointer)i);
		g_signal_connect_swapped(btn, "clicked", G_CALLBACK(gtk_widget_hide), (gpointer)_icon_window);
		gtk_widget_show(btn);
	}
	btn = gtk_button_new_with_label(" ... ");
	gtk_button_set_relief (GTK_BUTTON (btn), GTK_RELIEF_NONE);
	gtk_box_pack_start(GTK_BOX(hbox), btn, TRUE, TRUE, 1);
	g_signal_connect(btn, "clicked", G_CALLBACK(btn_clicked), (gpointer)XFCE_ICON_CATEGORY_EXTERN);
	g_signal_connect_swapped(btn, "clicked", G_CALLBACK(gtk_widget_hide), (gpointer)_icon_window);
	gtk_widget_show(btn);
	//XFCE_ICON_CATEGORY_STOCK
	
	gtk_widget_show(hbox);
	
	return _icon_window;
}


void
show_icon_window( GtkTreeView *treeview, GtkTreePath *arg1,
								GtkTreeViewColumn *arg2, gpointer user_data)
{
	if (gtk_tree_view_get_column(treeview, 0) == arg2)
	{
		gtk_window_set_position(GTK_WINDOW(_icon_window), GTK_WIN_POS_MOUSE);
		gtk_window_set_modal(GTK_WINDOW(_icon_window), TRUE);
		gtk_widget_show(_icon_window);
	}
}

gchar* get_icon_file()
{
	GtkWidget *chooser, *img;
	FileFilter *filter;
	gchar *result = NULL;
	chooser = file_chooser_new(_("Open icon"), GTK_WINDOW(_icon_window), FILE_CHOOSER_ACTION_OPEN,
												  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		      									  GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	/*Preview widget*/
	img = gtk_image_new();
	gtk_widget_set_size_request(img, 96, 96);
	gtk_widget_show(img);
	file_chooser_set_preview_widget(FILE_CHOOSER(chooser), img);
	file_chooser_set_preview_widget_active(FILE_CHOOSER(chooser), FALSE);
	file_chooser_set_preview_callback(FILE_CHOOSER(chooser), 
									file_chooser_preview_img, (gpointer)img);
	
	file_chooser_get_local_only(FILE_CHOOSER(chooser));
	file_chooser_set_select_multiple(FILE_CHOOSER(chooser), FALSE);
	filter = file_filter_new();
	if (filter){
		file_filter_set_name(filter, "image");
		file_filter_add_mime_type(filter, "image/*");
		file_chooser_add_filter(FILE_CHOOSER(chooser), filter);	
	}
	file_chooser_set_current_folder(FILE_CHOOSER(chooser), "/usr/share/pixmaps");//Maybe can be changed...
	gtk_window_set_modal(GTK_WINDOW(chooser), TRUE);
	gtk_window_set_transient_for(GTK_WINDOW(chooser),  _gtk_widget_get_parent_gtk_window(_dlg->vbox) );
	if(gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT)
		result = file_chooser_get_filename(FILE_CHOOSER(chooser));
	
	gtk_widget_destroy(img);
	gtk_widget_destroy(chooser);
	
	return result;
}

//Callback functions
//******************************************************************************
void
btn_clicked(GtkButton *button, gpointer icon_id)
{
	GtkTreeModel *treemodel;
	GtkTreeIter iter;	
	GdkPixbuf *pixbuf;
	GtkTreeSelection *sel;
	gchar *icon_name;
	t_launcher *launcher;
	
	sel = gtk_tree_view_get_selection( GTK_TREE_VIEW(_dlg->treeview1) );
	if ( gtk_tree_selection_get_selected(sel, &treemodel, &iter) )
	{
		gtk_tree_model_get(treemodel, &iter, 2, &launcher, -1);
		if ( (gint)icon_id == XFCE_ICON_CATEGORY_EXTERN )	
		{
			gtk_window_set_modal(GTK_WINDOW(_icon_window), FALSE);
			gtk_widget_hide(GTK_WIDGET(_icon_window));
			icon_name = get_icon_file();
			//gtk_widget_show(GTK_WIDGET(_icon_window)); //useless
			if (icon_name)
			{
				if (launcher->icon_name) 
					g_free(launcher->icon_name); 
				launcher->icon_name = icon_name;
				launcher->icon_id = (gint)icon_id;
			}
		}else
			launcher->icon_id = (gint)icon_id;
		launcher_update_icon(launcher);
		pixbuf = _create_pixbuf(launcher->icon_id, icon_name, 16); 
		gtk_list_store_set(GTK_LIST_STORE(treemodel), &iter, 0, pixbuf, -1);				
		UNREF(pixbuf);
	}
}

void on_spin_value_changed(GtkSpinButton *spinbutton, gpointer user_data)
{
	quicklauncher_set_nblines(gtk_spin_button_get_value_as_int(spinbutton));
}

void
on_btn_new_clicked (GtkButton *button, gpointer user_data)
{
	GtkTreeModel *treemodel;
	GtkTreeIter iter;
	GdkPixbuf *pixbuf; 
	t_launcher *launcher =  launcher_new (NULL, XFCE_ICON_CATEGORY_UNKNOWN, NULL);
	
	treemodel = gtk_tree_view_get_model(GTK_TREE_VIEW(_dlg->treeview1) );
	gtk_list_store_insert(GTK_LIST_STORE(treemodel), &iter, INT_MAX); //INT_MAX must be enough ;-)
	pixbuf = xfce_icon_theme_load_category(DEFAULT_ICON_THEME, XFCE_ICON_CATEGORY_UNKNOWN, 16); 
	gtk_list_store_set(GTK_LIST_STORE(treemodel), &iter, 0, pixbuf, 1, NULL ,2, (gpointer)launcher, -1);
	UNREF(pixbuf);
	quicklauncher_empty_widgets();
	quicklauncher_add_element(launcher);
	quicklauncher_organize();
}


void
on_btn_remove_clicked (GtkButton  *button, gpointer user_data)
{
	GtkTreeModel *treemodel;
	GtkTreeIter iter, previous;	
	GtkTreeSelection *sel;
	GtkTreePath *path;
	t_launcher *removed;
	gint *indice;
	
	sel = gtk_tree_view_get_selection( GTK_TREE_VIEW(_dlg->treeview1) );
	if ( gtk_tree_selection_get_selected(sel, &treemodel, &iter) )
	{
		path = gtk_tree_model_get_path(treemodel, &iter);
		indice = gtk_tree_path_get_indices(path);
		gtk_list_store_remove(GTK_LIST_STORE(treemodel), &iter);
		quicklauncher_empty_widgets();
		removed = quicklauncher_remove_element(indice[0]);
		quicklauncher_organize();
		launcher_free (removed);
		gtk_tree_path_free(path); 
	}
}

void
on_btn_up_clicked (GtkButton *button, gpointer user_data)
{
	GtkTreeModel *treemodel;
	GtkTreeIter iter, previous;	
	GtkTreeSelection *sel;
	GtkTreePath *path;
	gint i, *indice;
	GList *launcher;
	
	sel = gtk_tree_view_get_selection( GTK_TREE_VIEW(_dlg->treeview1) );
	if ( gtk_tree_selection_get_selected(sel, &treemodel, &iter) )
	{
		path = gtk_tree_model_get_path(treemodel, &iter);
		if ( gtk_tree_path_prev(path) )
		{
			if (gtk_tree_model_get_iter(treemodel, &previous, path))
				gtk_list_store_swap(GTK_LIST_STORE(treemodel), &iter, &previous);
			indice = gtk_tree_path_get_indices(path);
			launcher = g_list_nth(_quicklauncher->launchers,  indice[0]+1);
			_quicklauncher->launchers = g_list_remove_link(_quicklauncher->launchers, launcher);
			_quicklauncher->launchers = g_list_insert(_quicklauncher->launchers, launcher->data, indice[0]);
			quicklauncher_empty_widgets();
			quicklauncher_organize();
			g_list_free(launcher);
		}
		gtk_tree_path_free(path);
	}
}


void
on_btn_down_clicked (GtkButton *button, gpointer user_data)
{
	GtkTreeModel *treemodel;
	GtkTreeIter iter, next;	
	GtkTreeSelection *sel;
	GtkTreePath *path;
	gint i, *indice;
	GList *launcher;
	
	sel = gtk_tree_view_get_selection( GTK_TREE_VIEW(_dlg->treeview1) );
	if ( gtk_tree_selection_get_selected(sel, &treemodel, &iter) )
	{
		next = iter;
		if (gtk_tree_model_iter_next(treemodel, &next))
		{
			gtk_list_store_swap(GTK_LIST_STORE(treemodel), &iter, &next);
			path = gtk_tree_model_get_path(treemodel, &next);
			indice = gtk_tree_path_get_indices(path);
			launcher = g_list_nth(_quicklauncher->launchers,  indice[0]+1);
			_quicklauncher->launchers = g_list_remove_link(_quicklauncher->launchers, launcher);
			_quicklauncher->launchers = g_list_insert(_quicklauncher->launchers, launcher->data, indice[0]);
			quicklauncher_empty_widgets();
			quicklauncher_organize();
			gtk_tree_path_free (path);
			g_list_free(launcher);
		}
	}
}

void cmd_changed(GtkCellRendererText *cellrenderertext, gchar *arg1, gchar *arg2, gpointer user_data)
{
	GtkTreeSelection *sel;
	GtkTreeModel *treemodel;
	GtkTreePath *path;
	gint *indice;
	GtkTreeIter iter;
	t_launcher *launcher;
	
	sel = gtk_tree_view_get_selection( GTK_TREE_VIEW(_dlg->treeview1) );
	if ( gtk_tree_selection_get_selected(sel, &treemodel, &iter) )
	{
		path = gtk_tree_model_get_path(treemodel, &iter);
		indice = gtk_tree_path_get_indices(path);
		launcher = (t_launcher*) (g_list_nth(_quicklauncher->launchers,  indice[0]))->data;
		g_free(launcher->command);
		launcher->command = (gchar*) g_malloc(sizeof(gchar) * (strlen(arg2)+1));
		strcpy(launcher->command, arg2);
		gtk_list_store_set(GTK_LIST_STORE(treemodel), &iter, 1, launcher->command, -1);
		launcher_update_command(launcher);
		gtk_tree_path_free(path);
	}
}


void  file_chooser_preview_img (FileChooser *chooser, gpointer user_data)
{
	g_assert(GTK_IS_IMAGE(user_data));
	gchar *filename = file_chooser_get_filename(chooser);
	if(g_file_test(filename, G_FILE_TEST_IS_REGULAR))
	{
		file_chooser_set_preview_widget_active(chooser, TRUE);
		gtk_image_set_from_file( GTK_IMAGE(user_data), filename);
	} else
		file_chooser_set_preview_widget_active(chooser, FALSE);
	g_free(filename);
}
