/***************************************************************************
 *            main.c
 *
 *  Thu Jul 15 06:01:04 2004
 *  Last Update: 07/05/2005
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
#include <config.h>
#endif


#include "types.h"
#include "callbacks.h"



/* Quicklauncher funcs */

void quicklauncher_free(t_quicklauncher *quicklauncher);
gboolean quicklauncher_load_config(t_quicklauncher *quicklauncher, const gchar* filename);
void quicklauncher_save_config(t_quicklauncher *quicklauncher, const gchar* filename);

/* Launcher funcs */
t_launcher* launcher_load_config(XfceRc *rcfile, gint num, t_quicklauncher *quicklauncher);
void launcher_save_config(t_launcher *launcher, XfceRc *rcfile, guint16 num);

/* -------------------------------------------------------------------- *
 *                     Panel Plugin Interface                           *
 * -------------------------------------------------------------------- */

static void 
quicklauncher_construct (XfcePanelPlugin *plugin);

static void 
quicklauncher_orientation_changed(XfcePanelPlugin *plugin,
									GtkOrientation orientation,
                                    t_quicklauncher *quicklauncher);
static gboolean 
quicklauncher_set_size(XfcePanelPlugin *plugin,gint size,
						t_quicklauncher *quicklauncher);

static void 
quicklauncher_free_data(XfcePanelPlugin *plugin,t_quicklauncher *quicklauncher);

static void 
quicklauncher_save(XfcePanelPlugin *plugin, t_quicklauncher *quicklauncher);

static void 
quicklauncher_configure(XfcePanelPlugin *plugin, t_quicklauncher *quicklauncher);


XFCE_PANEL_PLUGIN_REGISTER_INTERNAL(quicklauncher_construct);

/* create widgets and connect to signals */
static void 
quicklauncher_construct (XfcePanelPlugin *plugin)
{
    t_quicklauncher *quicklauncher = quicklauncher_new(plugin);

    g_signal_connect (plugin, "orientation-changed", 
                      G_CALLBACK (quicklauncher_orientation_changed), quicklauncher);
    
    g_signal_connect (plugin, "size-changed", 
                      G_CALLBACK (quicklauncher_set_size), quicklauncher);
    
    g_signal_connect (plugin, "free-data", 
                      G_CALLBACK (quicklauncher_free_data), quicklauncher);
    
    g_signal_connect (plugin, "save", 
                      G_CALLBACK (quicklauncher_save), quicklauncher);
    
    xfce_panel_plugin_menu_show_configure (plugin);
    g_signal_connect (plugin, "configure-plugin", 
                      G_CALLBACK (quicklauncher_configure), quicklauncher);
}


void quicklauncher_orientation_changed(XfcePanelPlugin *plugin,
									GtkOrientation orientation,
                                    t_quicklauncher *quicklauncher)
{
	quicklauncher->orientation = orientation;
	quicklauncher_empty_widgets(quicklauncher);
	quicklauncher_organize(quicklauncher);
}

gboolean quicklauncher_set_size(XfcePanelPlugin *plugin, gint size,
							t_quicklauncher *quicklauncher)
{
	GList *liste;
	quicklauncher->icon_size = (int)size/quicklauncher->nb_lines;
	for(liste = quicklauncher->launchers;
		liste ; liste = g_list_next(liste) )
	{
		launcher_update_icon((t_launcher*)liste->data, quicklauncher->icon_size);		
		gtk_container_set_border_width( GTK_CONTAINER( ( (t_launcher*)liste->data)->widget), 
										(int)quicklauncher->icon_size/8);
	}
	return TRUE;
}

void quicklauncher_free_data(XfcePanelPlugin *plugin, t_quicklauncher *quicklauncher)
{
	quicklauncher_free(quicklauncher);
}

void quicklauncher_save(XfcePanelPlugin *plugin, t_quicklauncher *quicklauncher)
{
	gchar *filename;
	if(filename = xfce_panel_plugin_save_location(plugin))
	{
		quicklauncher_save_config(quicklauncher, filename);
		g_free(filename);
	}
}

void 
quicklauncher_configure(XfcePanelPlugin *plugin, t_quicklauncher *quicklauncher)
{
	xfce_panel_plugin_block_menu(plugin); 
	t_qck_launcher_opt_dlg* dlg = create_qck_launcher_dlg();
	qck_launcher_opt_dlg_set_quicklauncher(quicklauncher);
	gtk_dialog_run(GTK_DIALOG(dlg->dialog));
	xfce_panel_plugin_unblock_menu(plugin);
}


void
quicklauncher_add_element(t_quicklauncher *quicklauncher, t_launcher *launcher)
{
	quicklauncher->launchers = g_list_append(quicklauncher->launchers, (gpointer)launcher);
	xfce_panel_plugin_add_action_widget(quicklauncher->plugin, launcher->widget);
	quicklauncher->nb_launcher++; 
}


t_launcher*
quicklauncher_remove_element(t_quicklauncher *quicklauncher, gint num)
{
	t_launcher *result;
	GList *elem = g_list_nth(quicklauncher->launchers, num);
	quicklauncher->launchers = g_list_remove_link(quicklauncher->launchers, elem);
	quicklauncher->nb_launcher--;
	result = (t_launcher*)(elem->data);
	g_list_free_1(elem);
	return result;
}

void
quicklauncher_organize(t_quicklauncher *quicklauncher)
{
	gint i, j, launch_per_line, nb_lines;
	GList *toplace;
	
	g_assert( (!quicklauncher->table || GTK_IS_TABLE(quicklauncher->table)) && GTK_IS_CONTAINER(quicklauncher->plugin));
	if (quicklauncher->launchers) 
	{
		nb_lines = MIN(quicklauncher->nb_lines, quicklauncher->nb_launcher); 
		toplace = g_list_first(quicklauncher->launchers);
		if(quicklauncher->nb_launcher % quicklauncher->nb_lines == 0)
			launch_per_line = quicklauncher->nb_launcher / quicklauncher->nb_lines;
		else 
			launch_per_line = quicklauncher->nb_launcher / quicklauncher->nb_lines+1;
		if (quicklauncher->orientation != GTK_ORIENTATION_HORIZONTAL)
		{
			i = nb_lines;
			nb_lines = launch_per_line;
			launch_per_line = i;
		}
		if (quicklauncher->table)
			gtk_table_resize(GTK_TABLE(quicklauncher->table), nb_lines, launch_per_line);
		
		j = quicklauncher->nb_launcher;
		for (i=0; i < nb_lines; ++i)
		{
			for(j=0; (j < launch_per_line) && (toplace); ++j, toplace = g_list_next(toplace))
			{
				g_assert(toplace && GTK_IS_WIDGET(((t_launcher*)toplace->data)->widget) );
				gtk_table_attach_defaults( GTK_TABLE(quicklauncher->table),
											((t_launcher*)toplace->data)->widget,
											j, j+1, i, i+1);
				//gtk_container_add (GTK_CONTAINER (quicklauncher->hbox[i]), ((t_launcher*)toplace->data)->widget);
			}
		}
	}
}


void
quicklauncher_empty_widgets(t_quicklauncher *quicklauncher)
{
	GList *launcher;	
	if (quicklauncher->table)
	{
		for( launcher = g_list_first(quicklauncher->launchers);
			 launcher; launcher = g_list_next(launcher) )
			gtk_container_remove(GTK_CONTAINER(quicklauncher->table), 
								((t_launcher*)launcher->data)->widget);
	}	
}


void
quicklauncher_empty(t_quicklauncher *quicklauncher)
{
	int i;	
	quicklauncher_empty_widgets(quicklauncher);
	if (quicklauncher->launchers)
	{
		g_list_foreach (quicklauncher->launchers, (GFunc) launcher_free, NULL);
		g_list_free (quicklauncher->launchers);
		quicklauncher->launchers = NULL;
	}
	quicklauncher->nb_lines = 0;
	quicklauncher->nb_launcher = 0;
}


void 
quicklauncher_set_nblines(t_quicklauncher *quicklauncher, gint nb_lines)
{
	if (nb_lines != quicklauncher->nb_lines)
	{
		quicklauncher_empty_widgets(quicklauncher);
		quicklauncher->nb_lines = nb_lines;
		quicklauncher_set_size(quicklauncher->plugin,
			xfce_panel_plugin_get_size(quicklauncher->plugin),quicklauncher);
		quicklauncher_organize(quicklauncher);
	}
}


void
quicklauncher_load_default(t_quicklauncher *quicklauncher)
{
	t_launcher *launcher;
	quicklauncher->nb_lines = 2;
	
	launcher = launcher_new("xflock4", XFCE_ICON_CATEGORY_SYSTEM, 
							NULL, quicklauncher);
	quicklauncher_add_element(quicklauncher, launcher);
	launcher = launcher_new("xfce-setting-show", XFCE_ICON_CATEGORY_SETTINGS, 
							NULL, quicklauncher);
	quicklauncher_add_element(quicklauncher, launcher);
	launcher = launcher_new("xfce4-appfinder", XFCE_ICON_CATEGORY_UTILITY, 
							NULL, quicklauncher);
	quicklauncher_add_element(quicklauncher, launcher);
	launcher = launcher_new("xfhelp4", XFCE_ICON_CATEGORY_HELP, 
							NULL, quicklauncher);
	quicklauncher_add_element(quicklauncher, launcher);
	g_assert(quicklauncher->nb_launcher == 4);
}	


t_quicklauncher *
quicklauncher_new (XfcePanelPlugin *plugin)
{
	g_print("create quicklauncher\n");
	t_quicklauncher *quicklauncher;
	
	quicklauncher = g_new0(t_quicklauncher, 1);
	gchar *filename = xfce_panel_plugin_save_location(plugin);
	
	if((!filename) || (!quicklauncher_load_config(quicklauncher, filename) ) )
		quicklauncher_load_default(quicklauncher);
	
	quicklauncher->icon_size = (gint) xfce_panel_plugin_get_size(plugin)/2;
	quicklauncher->orientation = xfce_panel_plugin_get_orientation(plugin);
	quicklauncher->plugin = plugin;
	quicklauncher->table = g_object_ref(gtk_table_new(2, 2, TRUE));
	gtk_table_set_col_spacings(GTK_TABLE(quicklauncher->table), 0);
	gtk_container_add( GTK_CONTAINER(quicklauncher->plugin), quicklauncher->table);
	xfce_panel_plugin_add_action_widget(quicklauncher->plugin, quicklauncher->table);
	gtk_widget_show(quicklauncher->table);
	
	quicklauncher_organize(quicklauncher);
	return quicklauncher;
}


void
quicklauncher_free(t_quicklauncher *quicklauncher)
{
	int i;
	g_list_foreach(quicklauncher->launchers, (GFunc) launcher_free, NULL);
	g_list_free(quicklauncher->launchers);
	
	g_object_unref(quicklauncher->table);
	g_free(quicklauncher);
}


gboolean quicklauncher_load_config(t_quicklauncher *quicklauncher, const gchar* filename)
{
	
	XfceRc* rcfile;
	if( rcfile = xfce_rc_simple_open(filename, TRUE) )
	{
		xfce_rc_set_group(rcfile, NULL);
		quicklauncher->nb_lines = xfce_rc_read_int_entry(rcfile, "nb_lines", 1);
		gint i = xfce_rc_read_int_entry(rcfile, "nb_launcher", 0);
		g_assert(i >= 0);
		while(i)
		{
			t_launcher *launcher = launcher_load_config(rcfile, i, quicklauncher);
			quicklauncher_add_element(quicklauncher, launcher);
			if(!i--) return TRUE;
		}
	} else
	return FALSE;
}

void
quicklauncher_save_config(t_quicklauncher *quicklauncher, const gchar* filename)
{
	guint16 i = quicklauncher->nb_launcher; //hope it always works
	XfceRc* rcfile = xfce_rc_simple_open(filename, FALSE);
	if(!rcfile) return;
	
	xfce_rc_set_group(rcfile, NULL);
	xfce_rc_write_int_entry(rcfile, "nb_lines", quicklauncher->nb_lines);
	xfce_rc_write_int_entry(rcfile, "nb_launcher", quicklauncher->nb_launcher);
	xfce_rc_flush(rcfile);
	GList* liste;
	for( liste = quicklauncher->launchers; liste; liste = g_list_next(liste), --i)
		launcher_save_config((t_launcher*)liste->data, rcfile, i);
	
	g_assert(i == 0);
	xfce_rc_close(rcfile);
}

/* -------------------------------------------------------------------- *
 *                        Launcher Interface                            *
 * -------------------------------------------------------------------- */

//TO DO: support XFCE icon by name
GdkPixbuf *
_create_pixbuf(gint id, const gchar* name, gint size)
{
	GdkPixbuf  *pixbuf;
	if(id != XFCE_ICON_CATEGORY_EXTERN)
		pixbuf = xfce_icon_theme_load_category(DEFAULT_ICON_THEME, id, size);
	else
		pixbuf = gdk_pixbuf_new_from_file_at_size(name, size, size, NULL);
	if(!pixbuf)
		pixbuf = xfce_icon_theme_load_category(DEFAULT_ICON_THEME, XFCE_ICON_CATEGORY_UNKNOWN, size);
	return pixbuf;
}

gboolean
launcher_clicked (GtkWidget *event_box, GdkEventButton *event, t_launcher *launcher)
{
	int size = 1.25 * launcher->quicklauncher->icon_size;
	if (event->button != 1) 
		return FALSE;
	if (event->type == GDK_BUTTON_PRESS) 
	{ 
		g_assert(launcher->zoomed_img);
		if(event->x < 0 || event->x > size || event->y < 0 || event->y > size)
			return FALSE;
		if (!launcher->clicked_img)
		{
			launcher->clicked_img = gdk_pixbuf_copy (launcher->zoomed_img);
			gdk_pixbuf_saturate_and_pixelate(launcher->zoomed_img, launcher->clicked_img, 5, TRUE);
		}
		gtk_image_set_from_pixbuf (GTK_IMAGE(launcher->image), launcher->clicked_img);
	}
	else if (event->type == GDK_BUTTON_RELEASE)
	{
		g_assert(launcher->clicked_img);
		if (event->x > 0 && event->x < size && event->y > 0 && event->y < size)
			xfce_exec(launcher->command, FALSE, FALSE, NULL);
		gtk_image_set_from_pixbuf (GTK_IMAGE(launcher->image), launcher->def_img);
		gtk_container_set_border_width(GTK_CONTAINER (event_box),
										(int)launcher->quicklauncher->icon_size/8);
		//gtk_widget_set_size_request(launcher->image, size, size);
	}
	return TRUE;
}

gboolean    
launcher_passthrought(GtkWidget *widget, GdkEventCrossing *event, t_launcher *launcher)
{
	if (event->type == GDK_ENTER_NOTIFY)
	{
		int size = 1.25 * launcher->quicklauncher->icon_size;
		if (!launcher->zoomed_img)
			launcher->zoomed_img = gdk_pixbuf_scale_simple(launcher->def_img, size, size, GDK_INTERP_BILINEAR);
		gtk_container_set_border_width(GTK_CONTAINER (widget), 0);
		gtk_image_set_from_pixbuf (GTK_IMAGE(launcher->image), launcher->zoomed_img);
	}
	else 
	{
		gtk_image_set_from_pixbuf (GTK_IMAGE(launcher->image), launcher->def_img);
		gtk_container_set_border_width(GTK_CONTAINER (widget), 
										(int)(launcher->quicklauncher->icon_size/8));
	}
	return TRUE;
}


void 
launcher_update_icon(t_launcher *launcher, gint size) 
{
	UNREF(launcher->def_img);
	UNREF(launcher->zoomed_img); launcher->zoomed_img = NULL;
	UNREF(launcher->clicked_img); launcher->clicked_img = NULL;
	launcher->def_img = _create_pixbuf(launcher->icon_id, launcher->icon_name, size);
	if (launcher->def_img)
		gtk_image_set_from_pixbuf(GTK_IMAGE(launcher->image), launcher->def_img);
	gtk_widget_set_size_request(launcher->image, size, size);
}

void 
launcher_update_icons(gpointer data, gint* size)
{
	launcher_update_icon((t_launcher*)data, *size); //used with g_list_foreach as a GFunc =>could be removed
}


void 
launcher_update_command(t_launcher *launcher) 
{
	if (launcher->command_ids[0] )
	{
		g_signal_handler_disconnect(launcher->widget, launcher->command_ids[0]);
		g_signal_handler_disconnect(launcher->widget, launcher->command_ids[1]);
		g_signal_handler_disconnect(launcher->widget, launcher->command_ids[2]);
		g_signal_handler_disconnect(launcher->widget, launcher->command_ids[3]);
	}
	gtk_tooltips_set_tip(launcher->tooltip, launcher->widget, launcher->command, launcher->command);
	launcher->command_ids[0] = g_signal_connect(launcher->widget, "button_press_event", 
												G_CALLBACK(launcher_clicked), launcher);
	launcher->command_ids[1] = g_signal_connect(launcher->widget, "button-release-event", 
												G_CALLBACK(launcher_clicked), launcher);
	launcher->command_ids[2] = g_signal_connect(launcher->widget, "enter-notify-event", 
												G_CALLBACK(launcher_passthrought), launcher);
	launcher->command_ids[3] = g_signal_connect(launcher->widget, "leave-notify-event", 
												G_CALLBACK(launcher_passthrought), launcher);
}

void create_launcher(t_launcher	*launcher)
{	
	launcher->widget = g_object_ref(gtk_event_box_new());
	launcher->image = g_object_ref(gtk_image_new());
	launcher->tooltip = gtk_tooltips_new();
	gtk_container_set_border_width(GTK_CONTAINER (launcher->widget), 
								(int)launcher->quicklauncher->icon_size/8);
	gtk_container_add (GTK_CONTAINER (launcher->widget), launcher->image);
	gtk_event_box_set_above_child(GTK_EVENT_BOX(launcher->widget), FALSE);
	
	launcher_update_icon(launcher, launcher->quicklauncher->icon_size);
	g_assert(!launcher->command_ids[0]);
	launcher_update_command(launcher) ;
	gtk_widget_show (launcher->image);
	gtk_widget_show (launcher->widget);
}

t_launcher *
launcher_new (const gchar *command, gint icon_id, const gchar *icon_name, t_quicklauncher* quicklauncher)
{
    t_launcher *launcher;
	launcher = g_new0 (t_launcher, 1);
	
    if(command)
	{
		launcher->command = g_malloc( (strlen(command)+1)*sizeof(gchar) );
		launcher->command = strcpy(launcher->command, command);
	}
	else launcher->command = NULL;
	launcher->icon_id = icon_id;
	if (icon_name)
	{
		launcher->icon_name = g_malloc( (strlen(icon_name)+1)*sizeof(gchar) );
		launcher->icon_name =strcpy(launcher->icon_name, icon_name);
	}
	else launcher->icon_name = NULL; 	
	launcher->quicklauncher = quicklauncher;
	create_launcher(launcher);
	return launcher;
}

void
launcher_free (t_launcher *launcher)
{
	if(!launcher) return;
	UNREF(launcher->def_img);
	UNREF(launcher->zoomed_img);
	UNREF(launcher->clicked_img); 
	g_object_unref(launcher->tooltip);
	g_object_unref(launcher->widget);
	g_object_unref(launcher->image);
	
	//gtk_widget_destroy(launcher->widget); //useless: handled by gtk
	g_free(launcher->icon_name);
	g_free(launcher->command);
	
    g_free (launcher);
}

t_launcher*
launcher_load_config(XfceRc *rcfile, gint num, t_quicklauncher *quicklauncher)
{
	char group[15];
	t_launcher *launcher;
	g_sprintf(group, "launcher_%d\0", num); 
	xfce_rc_set_group(rcfile, group);
	
	launcher = g_new0 (t_launcher, 1);
	launcher->quicklauncher = quicklauncher;
	launcher->command = g_strdup(xfce_rc_read_entry(rcfile, "command", NULL));
	launcher->icon_name = g_strdup(xfce_rc_read_entry(rcfile, "icon_name", NULL));
	launcher->icon_id = xfce_rc_read_int_entry(rcfile, "icon_id", 0);
	
	create_launcher(launcher);
	return launcher;
}

void 
launcher_save_config(t_launcher *launcher, XfceRc *rcfile, guint16 num)
{
	char group[15];
	g_sprintf(group, "launcher_%d\0", num); 
	xfce_rc_set_group(rcfile, group);
	xfce_rc_write_entry(rcfile, "command", launcher->command);
	if(launcher->icon_name)
		xfce_rc_write_entry(rcfile, "icon_name", launcher->icon_name);
	xfce_rc_write_int_entry(rcfile, "icon_id", launcher->icon_id);
	xfce_rc_flush(rcfile);
}
