/***************************************************************************
 *            main.c
 *
 *  Thu Jul 15 06:01:04 2004
 *  Last Update: 07/02/2005
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

//must be somewhere :)
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

void
launcher_clicked(GtkWidget *widget, gchar *command) 
{
	xfce_exec(command, FALSE, FALSE, NULL);
}

void 
launcher_update_icon(t_launcher *launcher) 
{
	GdkPixbuf *pixbuf = _create_pixbuf(launcher->icon_id, launcher->icon_name, _quicklauncher->icon_size);
	if (pixbuf)
	{
		xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON (launcher->widget), pixbuf);
		g_object_unref (pixbuf);
	}
}

void 
launcher_update_icons(gpointer data, gpointer user_data)
{
	launcher_update_icon((t_launcher*)data); //used with g_list_foreach as a GFunc 
}


void 
launcher_update_command(t_launcher *launcher) 
{
	if(launcher->command_id)
		 g_signal_handler_disconnect(launcher->widget, launcher->command_id);
	if (launcher->command)
		launcher->command_id = g_signal_connect(launcher->widget, "clicked",
																			    G_CALLBACK(launcher_clicked), launcher->command);
	else
		launcher->command_id = 0;
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
	//NOTE : I affect the name of the icon even if she's useless for now
	if (icon_name)
	{
		launcher->icon_name = g_malloc( (strlen(icon_name)+1)*sizeof(gchar) );
		launcher->icon_name =strcpy(launcher->icon_name, icon_name);
	}
	else launcher->icon_name = NULL; 	
	launcher->widget = g_object_ref(xfce_iconbutton_new());
	
	gtk_button_set_relief (GTK_BUTTON (launcher->widget), GTK_RELIEF_NONE);
	launcher_update_icon(launcher) ;
	launcher->command_id = 0;
	launcher_update_command(launcher) ;
	gtk_widget_show (launcher->widget);
	
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
	launcher->widget = g_object_ref(xfce_iconbutton_new ());
	
	gtk_button_set_relief (GTK_BUTTON (launcher->widget), GTK_RELIEF_NONE);
	launcher_update_icon(launcher) ;
	launcher->command_id = 0;
	launcher_update_command(launcher) ;
	gtk_widget_show (launcher->widget);
	
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
	g_object_unref(launcher->widget);
	//gtk_widget_destroy(launcher->widget); //useless: handled by gtk
	g_free(launcher->icon_name);
	g_free(launcher->command);
	
    g_free (launcher);
}

static void 
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

static  void
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

static  t_quicklauncher *
quicklauncher_new (GtkWidget *base)
{
	t_launcher *new_launcher;//temporaire
	_quicklauncher = g_new0(t_quicklauncher, 1);
	//appel temporaire
	_quicklauncher->nb_lines = 1;
	new_launcher = launcher_new("xflock4", XFCE_ICON_CATEGORY_SYSTEM, NULL);
	quicklauncher_add_element(new_launcher);
	new_launcher = launcher_new("xfce-setting-show", XFCE_ICON_CATEGORY_SETTINGS, NULL);
	quicklauncher_add_element(new_launcher);
	new_launcher = launcher_new("xmms", XFCE_ICON_CATEGORY_SOUND, NULL);
	quicklauncher_add_element(new_launcher);
	_quicklauncher->icon_size = 16;
	_quicklauncher->orientation = HORIZONTAL;
	g_assert(_quicklauncher->nb_launcher == 3);
	//fin temporaire
	_quicklauncher->base = base;
	quicklauncher_organize();
	return _quicklauncher;
}

static void
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
		_quicklauncher->nb_lines = MAX(atoi(value), 2);
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


static void
quicklauncher_write_config(Control * control, xmlNodePtr node)
{
	char value[3];
	GList *iter;
	xmlNodePtr launcher_node;
	
	g_assert(_quicklauncher == control->data);
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

static void
quicklauncher_configure(GtkContainer *container, GtkWidget *done)
{
	create_qck_launcher_dlg();
	fill_qck_launcher_dlg();
	gtk_container_add (container, _dlg->hbox1);
	//g_signal_connect(done, "clicked", G_CALLBACK (apply_config), (gpointer)quicklauncher);
	//g_signal_connect_swapped(done, "destroy", G_CALLBACK (g_free), (gpointer)dlg);
	g_signal_connect_swapped(done, "destroy", G_CALLBACK (free_qck_launcher_dlg), NULL);
}


static void
quicklauncher_set_size(gint size)
{
	if (size > SMALL) 
	{
		_quicklauncher->nb_lines = 3;
		_quicklauncher->icon_size = (gint)icon_size[size] / 2.5;
	}
	else 
	{
		_quicklauncher->nb_lines = 2;
		_quicklauncher->icon_size = (gint)icon_size[size] / 1.5;
	}
}

static gboolean
create_plugin (Control * control)
{
    control->data = quicklauncher_new(control->base);
	control->with_popup = FALSE;//Not implemented yet
    return TRUE;
}

static void
free_plugin(Control * control)
{
	quicklauncher_free();
}


static void
plugin_attach_callback (Control * control, const char *signal,
										GCallback callback, gpointer data)
{
	GList *iter;
	//also taken from systemsbuttons.c
	SignalCallback *cb;
	g_assert(_quicklauncher  == control->data);
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


static void
plugin_load_config(Control * control, xmlNodePtr node)
{
	g_assert(_quicklauncher  == control->data);
	gtk_container_remove (GTK_CONTAINER (control->base), _quicklauncher->table);
	quicklauncher_load_config(node);
	gtk_container_add (GTK_CONTAINER (control->base), _quicklauncher->table);
}

static void
plugin_create_options (Control * control, GtkContainer * container,
								      GtkWidget * done)
{
	g_assert(_quicklauncher  == control->data);
	quicklauncher_configure(container, done);
}


static void
plugin_set_size (Control * control, int size)
{
	g_assert(_quicklauncher  == control->data);
	quicklauncher_set_size(size);
	gtk_widget_set_size_request (_quicklauncher->table, -1, -1);
	quicklauncher_empty_widgets();
	quicklauncher_organize();
}


static void
plugin_set_orientation(Control * control, int orientation)
{
	_quicklauncher->orientation = orientation;
	quicklauncher_empty_widgets();
	quicklauncher_organize();
}


static void
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
