/***************************************************************************
 *            main.c
 *
 *  Thu Jul 15 06:01:04 2004
 *  Last Update: 04/04/2005
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

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/xfce_iconbutton.h>
#include <panel/xfce.h>
#include <panel/plugins.h>

#include "types.h"
#include "callbacks.h"

t_quicklauncher *_quicklauncher;

//TO DO: support XFCE icon by name
GdkPixbuf *
_create_pixbuf(gint id, const gchar* name, gint size)
{
	GdkPixbuf  *pixbuf;
	if(id != XFCE_ICON_CATEGORY_EXTERN)
		pixbuf = xfce_icon_theme_load_category(DEFAULT_ICON_THEME, id, size);
	else
		pixbuf = gdk_pixbuf_new_from_file_at_size(name, size, size, NULL);//hope it works with NULL
	if(!pixbuf)
		pixbuf = xfce_icon_theme_load_category(DEFAULT_ICON_THEME, XFCE_ICON_CATEGORY_UNKNOWN, size);
	return pixbuf;
}

gboolean
launcher_clicked (GtkWidget *event_box, GdkEventButton *event, t_launcher *launcher)
{
	int size =_quicklauncher->icon_size +2 * _quicklauncher->panel_size;
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
	}
	return TRUE;
}

gboolean    
launcher_passthrought(GtkWidget *widget, GdkEventCrossing *event, t_launcher *launcher)
{
	if (event->type == GDK_ENTER_NOTIFY)
	{
		if (!launcher->zoomed_img)
		{
			int size;
			size = _quicklauncher->icon_size +3*_quicklauncher->panel_size;
			launcher->zoomed_img = gdk_pixbuf_scale_simple(launcher->def_img, size, size, GDK_INTERP_BILINEAR);
		}
		gtk_container_set_border_width(GTK_CONTAINER (widget), 0);
		gtk_image_set_from_pixbuf (GTK_IMAGE(launcher->image), launcher->zoomed_img);
	}
	else 
	{
		gtk_image_set_from_pixbuf (GTK_IMAGE(launcher->image), launcher->def_img);
		gtk_container_set_border_width(GTK_CONTAINER (widget), 2*_quicklauncher->panel_size);
	}
	return TRUE;
}


void 
launcher_update_icon(t_launcher *launcher) 
{
	UNREF(launcher->def_img);
	UNREF(launcher->zoomed_img); launcher->zoomed_img = NULL;
	UNREF(launcher->clicked_img); launcher->clicked_img = NULL;
	launcher->def_img = _create_pixbuf(launcher->icon_id, launcher->icon_name, _quicklauncher->icon_size);
	if (launcher->def_img)
		gtk_image_set_from_pixbuf(GTK_IMAGE(launcher->image), launcher->def_img);
	gtk_widget_set_size_request(launcher->image, _quicklauncher->icon_size, _quicklauncher->icon_size);
}

void 
launcher_update_icons(gpointer data, gpointer user_data)
{
	launcher_update_icon((t_launcher*)data); //used with g_list_foreach as a GFunc 
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
	launcher->command_ids[0] = g_signal_connect(	launcher->widget, "button_press_event", 
																					G_CALLBACK(launcher_clicked), launcher);
	launcher->command_ids[1] = g_signal_connect(	launcher->widget, "button-release-event", 
																					G_CALLBACK(launcher_clicked), launcher);
	launcher->command_ids[2] = g_signal_connect(	launcher->widget, "enter-notify-event", 
																					G_CALLBACK(launcher_passthrought), launcher);
	launcher->command_ids[3] = g_signal_connect(	launcher->widget, "leave-notify-event", 
																					G_CALLBACK(launcher_passthrought), launcher);
}

void create_launcher(t_launcher	*launcher)
{	
	launcher->widget = g_object_ref(gtk_event_box_new());
	launcher->image = g_object_ref(gtk_image_new());
	launcher->tooltip = gtk_tooltips_new();
	gtk_container_set_border_width(GTK_CONTAINER (launcher->widget), 2*_quicklauncher->panel_size);
	gtk_container_add (GTK_CONTAINER (launcher->widget), launcher->image);
	gtk_event_box_set_above_child(GTK_EVENT_BOX(launcher->widget), FALSE);
	
	launcher_update_icon(launcher) ;
	g_assert(!launcher->command_ids[0]);
	launcher_update_command(launcher) ;
	gtk_widget_show (launcher->image);
	gtk_widget_show (launcher->widget);
}

 t_launcher *
launcher_new (const gchar *command, gint icon_id, const gchar *icon_name)
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
	create_launcher(launcher);
	return launcher;
}


t_launcher *
launcher_new_from_xml(xmlNodePtr node)
{
	t_launcher *launcher;
	gchar *tmp;
	launcher = g_new0 (t_launcher, 1);
	launcher->command = (gchar*) xmlGetProp (node, (const xmlChar *) "command");
	launcher->icon_name =(gchar*)  xmlGetProp (node, (const xmlChar *) "icon");
	tmp = (gchar*)  xmlGetProp (node, (const xmlChar *) "iconID") ;
	if (tmp)
	{
		launcher->icon_id = atoi(tmp);
		g_free((xmlChar *) tmp); 
	}else 
		launcher->icon_id = EXTERN_ICON;
	
	create_launcher(launcher);
	return launcher;
}


xmlNodePtr
launcher_save_to_xml(t_launcher *launcher)
{
	xmlNodePtr ptr;
	gchar buffer[17];
	if (!launcher) return NULL;
	ptr = xmlNewNode(NULL, (const xmlChar *)"launcher");
	xmlSetProp (ptr, (const xmlChar *) "command", (xmlChar *)launcher->command);
	xmlSetProp (ptr, (const xmlChar *) "icon", launcher->icon_name);
	g_sprintf(buffer,"%d\0",launcher->icon_id);  //The \0 stand for NULL who allows me to stop the string here
	xmlSetProp (ptr, (const xmlChar *) "iconID", buffer);
	
	return ptr;
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

void 
quicklauncher_connect_callbacks_to_launcher(t_launcher *launcher)
{
	GList *iter;
	SignalCallback *cb;
	for(iter=_quicklauncher->callback_data; iter; iter=g_list_next(iter))
	{
		cb = (SignalCallback *)iter->data;
		g_signal_connect(launcher->widget, cb->signal, cb->callback, cb->data);
	}
}

void
quicklauncher_reconnect_callbacks()
{
	GList *iter;
	for (iter = _quicklauncher->launchers; iter; iter = g_list_next(iter))
	{
		quicklauncher_connect_callbacks_to_launcher((t_launcher*)iter->data);
	}
}


void
quicklauncher_add_element( t_launcher *launcher )
{
	_quicklauncher->launchers = g_list_append( _quicklauncher->launchers, (gpointer)launcher);
	quicklauncher_connect_callbacks_to_launcher( launcher);
	_quicklauncher->nb_launcher++; 
}


t_launcher*
quicklauncher_remove_element(gint num)
{
	t_launcher *result;
	GList *elem = g_list_nth(_quicklauncher->launchers, num);
	_quicklauncher->launchers = g_list_remove_link(_quicklauncher->launchers, elem);
	_quicklauncher->nb_launcher--;
	result = (t_launcher*)(elem->data);
	g_list_free_1(elem);
	return result;
}

void
quicklauncher_organize()
{
	gint i, j, launch_per_line, nb_lines;
	GList *toplace;
	
	g_assert( (!_quicklauncher->table || GTK_IS_TABLE(_quicklauncher->table)) && GTK_IS_CONTAINER(_quicklauncher->base) );
	if (_quicklauncher->launchers) 
	{
		nb_lines = MIN(_quicklauncher->nb_lines, _quicklauncher->nb_launcher); 
		toplace = g_list_first(_quicklauncher->launchers);
		if(_quicklauncher->nb_launcher % _quicklauncher->nb_lines == 0)
			launch_per_line = _quicklauncher->nb_launcher / _quicklauncher->nb_lines;
		else 
			launch_per_line = _quicklauncher->nb_launcher / _quicklauncher->nb_lines+1;
		if (_quicklauncher->orientation != HORIZONTAL)
		{
			i = nb_lines;
			nb_lines = launch_per_line;
			launch_per_line = i;
		}
		if (_quicklauncher->table)
			gtk_table_resize(GTK_TABLE(_quicklauncher->table), nb_lines, launch_per_line);
		else
		{
			_quicklauncher->table = g_object_ref(gtk_table_new(nb_lines, launch_per_line, TRUE));
			gtk_table_set_col_spacings(GTK_TABLE(_quicklauncher->table), 0);
			gtk_container_add (GTK_CONTAINER (_quicklauncher->base), _quicklauncher->table);
			gtk_widget_show(_quicklauncher->table);
		}
		j = _quicklauncher->nb_launcher;
		for (i=0; i < nb_lines; ++i)
		{
			for(j=0; (j < launch_per_line) && (toplace); ++j, toplace = g_list_next(toplace))
			{
				g_assert(toplace && GTK_IS_WIDGET(((t_launcher*)toplace->data)->widget) );
				gtk_table_attach_defaults( GTK_TABLE(_quicklauncher->table),
															((t_launcher*)toplace->data)->widget,
															j, j+1, i, i+1);
				//gtk_container_add (GTK_CONTAINER (quicklauncher->hbox[i]), ((t_launcher*)toplace->data)->widget);
			}
		}
	}
}

t_quicklauncher *
quicklauncher_new (GtkWidget *base)
{
	t_launcher *new_launcher;
	_quicklauncher = g_new0(t_quicklauncher, 1);
	//Default
	_quicklauncher->nb_lines = 2;
	_quicklauncher->icon_size = 16;
	_quicklauncher->orientation = HORIZONTAL; //how can i grab these value from the panel?
	new_launcher = launcher_new("xflock4", XFCE_ICON_CATEGORY_SYSTEM, NULL);
	quicklauncher_add_element(new_launcher);
	new_launcher = launcher_new("xfce-setting-show", XFCE_ICON_CATEGORY_SETTINGS, NULL);
	quicklauncher_add_element(new_launcher);
	new_launcher = launcher_new("xfce4-appfinder", XFCE_ICON_CATEGORY_UTILITY, NULL);
	quicklauncher_add_element(new_launcher);
	new_launcher = launcher_new("xfhelp4", XFCE_ICON_CATEGORY_HELP, NULL);
	quicklauncher_add_element(new_launcher);
	g_assert(_quicklauncher->nb_launcher == 4);
	
	_quicklauncher->base = base;
	quicklauncher_organize();
	return _quicklauncher;
}

void
quicklauncher_free()
{
	int i;
	//next 2 line are taken from systembuttons.c
	g_list_foreach (_quicklauncher->callback_data, (GFunc) g_free, NULL);
	g_list_free (_quicklauncher->callback_data);
	g_list_foreach (_quicklauncher->launchers, (GFunc) launcher_free, NULL);
	g_list_free (_quicklauncher->launchers);
	
	g_object_unref(_quicklauncher->table);
	g_free(_quicklauncher);
}

void
quicklauncher_empty_widgets()
{
	GList *launcher;	
	if (_quicklauncher->table)
	{
		for( launcher = g_list_first(_quicklauncher->launchers);
				launcher; launcher = g_list_next(launcher) )
			gtk_container_remove(GTK_CONTAINER(_quicklauncher->table), 
												((t_launcher*)launcher->data)->widget);
	}	
}

void
quicklauncher_empty()
{
	int i;	
	quicklauncher_empty_widgets();
	if (_quicklauncher->launchers)
	{
		g_list_foreach (_quicklauncher->launchers, (GFunc) launcher_free, NULL);
		g_list_free (_quicklauncher->launchers);
		_quicklauncher->launchers = NULL;
	}
	_quicklauncher->nb_lines = 0;
	_quicklauncher->nb_launcher = 0;
}


void
quicklauncher_load_config(xmlNodePtr node)
{
	xmlChar *value;
	if (!node) return;
	quicklauncher_empty();
	value =  xmlGetProp (node, (const xmlChar *) "lines");
	if (value)
	{
		quicklauncher_set_nblines(atoi(value));
		xmlFree(value);
	}
	for (node = node->children; node; node = node->next)
    {
		//charger tous les lanceurs
		quicklauncher_add_element(launcher_new_from_xml(node));
	}
	quicklauncher_organize();
	quicklauncher_reconnect_callbacks();
}


void
quicklauncher_write_config(Control * control, xmlNodePtr node)
{
	char value[3];
	GList *iter;
	xmlNodePtr launcher_node;
	
//	g_assert(_quicklauncher == control->data);
	sprintf(value,"%d", _quicklauncher->nb_lines);
	xmlSetProp(node, (const xmlChar *)"lines", value);
	if(_quicklauncher->launchers) 
	{
		for (iter=_quicklauncher->launchers; iter; iter=g_list_next(iter))
		{
			launcher_node = launcher_save_to_xml((t_launcher*)iter->data);
			if (launcher_node) xmlAddChild(node, launcher_node);
		};
	}
}

void
quicklauncher_configure(GtkContainer *container, GtkWidget *done)
{
	create_qck_launcher_dlg();
	fill_qck_launcher_dlg();
	
	gtk_container_add (container, _dlg->vbox);
	g_signal_connect_swapped(done, "destroy", G_CALLBACK (free_qck_launcher_dlg), NULL);
}

void
quicklauncher_set_size(gint size)
{
	GList *liste;
	_quicklauncher->panel_size = size;
	if (size >= LARGE)
		_quicklauncher->icon_size = (gint) (icon_size[size] / _quicklauncher->nb_lines);
	else
		_quicklauncher->icon_size = (gint) (icon_size[size] / _quicklauncher->nb_lines) *1.25;
	for(liste = _quicklauncher->launchers;
		  liste ; liste = g_list_next(liste) )
	{
		launcher_update_icon((t_launcher*)liste->data);		
		gtk_container_set_border_width(GTK_CONTAINER (((t_launcher*)liste->data)->widget), 2*_quicklauncher->panel_size);
	}
}

void 
quicklauncher_set_nblines(gint nb_lines)
{
	if (nb_lines != _quicklauncher->nb_lines)
	{
		quicklauncher_empty_widgets();
		_quicklauncher->nb_lines = nb_lines;
		quicklauncher_set_size(_quicklauncher->panel_size);
		quicklauncher_organize();
	}
}

gboolean
create_plugin (Control * control)
{
    control->data = quicklauncher_new(control->base);
	control->with_popup = FALSE;//Not implemented yet
    return TRUE;
}

void
free_plugin(Control * control)
{
	quicklauncher_free();
}


void
plugin_attach_callback (Control * control, const char *signal,
										GCallback callback, gpointer data)
{
	GList *iter;
	//also taken from systemsbuttons.c
	SignalCallback *cb;
	//g_assert(_quicklauncher  == control->data);
	cb = g_new0 (SignalCallback, 1);
	cb->signal = signal;
	cb->callback = callback;
	cb->data = data;
	_quicklauncher->callback_data = g_list_append (_quicklauncher->callback_data, cb);
	
	g_signal_connect (_quicklauncher->table, signal, callback, data);
	for ( iter = g_list_first(_quicklauncher->launchers); iter; iter = g_list_next(iter) )
	{
		g_signal_connect ( ( (t_launcher*)iter->data)->widget, signal, callback, data);
	};
}


void
plugin_load_config(Control * control, xmlNodePtr node)
{
	//g_assert(_quicklauncher  == control->data);
	gtk_container_remove (GTK_CONTAINER (control->base), _quicklauncher->table);
	quicklauncher_load_config(node);
	gtk_container_add (GTK_CONTAINER (control->base), _quicklauncher->table);
}

void
plugin_create_options (Control * control, GtkContainer * container,
								      GtkWidget * done)
{
	//g_assert(_quicklauncher  == control->data);
	quicklauncher_configure(container, done);
}


void
plugin_set_size (Control * control, int size)
{
	//g_assert(_quicklauncher  == control->data);
	quicklauncher_set_size(size);
}


void
plugin_set_orientation(Control * control, int orientation)
{
	_quicklauncher->orientation = orientation;
	quicklauncher_empty_widgets();
	quicklauncher_organize();
}


void
plugin_set_theme (Control * control, const char *theme)
{
	quicklauncher_empty_widgets();
	g_list_foreach(_quicklauncher->launchers, launcher_update_icons, NULL);
	quicklauncher_organize();
}


G_MODULE_EXPORT void
xfce_control_class_init (ControlClass * cc)
{
    cc->name = "quicklauncher";
    cc->caption = "Quicklauncher";
    cc->create_control = (CreateControlFunc) create_plugin;
    cc->attach_callback = plugin_attach_callback;
    cc->free = free_plugin;
    cc->read_config = plugin_load_config;
    cc->write_config = quicklauncher_write_config; 
    cc->create_options = plugin_create_options;
	cc->set_size = plugin_set_size;
	cc->set_orientation = plugin_set_orientation;
    cc->set_theme = plugin_set_theme;
	
	control_class_set_unique (cc, TRUE);
}

/* Macro that checks panel API version */
XFCE_PLUGIN_CHECK_INIT
