/***************************************************************************
 *            types.h
 *
 *  Thu Jul 15 06:01:04 2004
 *  Last Update: 17/03/2005
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

#ifndef __QCK_LAUNCHER_TYPES_H__
#define __QCK_LAUNCHER_TYPES_H__


#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/xfce_iconbutton.h>
#include <panel/xfce.h>
#include <panel/plugins.h>


//#ifdef NEED_NAMES 
 	#define SetGtkName(wdg,name)	(gtk_widget_set_name ((GTK_WIDGET(wdg)), (name)))
//#else
//	#define SetGtkName(wdg,name)	
//#endif

#define XFCE_ICON_CATEGORY_STOCK 		(XFCE_N_BUILTIN_ICON_CATEGORIES + 1) //not used yet
#define XFCE_ICON_CATEGORY_EXTERN		(XFCE_N_BUILTIN_ICON_CATEGORIES + 2)
//can't you add these 2 things in xfce?? 
//Please also note than the file icon.h in the panel directory has his
//own declaration, wich is a bit confusing. (Moreover, they are not sync 
//with these from xfce_icontheme.h)

// It would be great to define this somewhere .I don't think it is the case
// just send me a mail if i'm wrong  :-)
static char *icons_categories_names[XFCE_N_BUILTIN_ICON_CATEGORIES+2] = 
	{"Unknown", "Editor", "Filemanager", "Utilities", "Games", "Help", "Multimedia", "Internet", "Graphics", \
	 "Printer", "Productivity", "Sound", "Terminal", "Development", "Settings", "System", "Wine", "Stock", "Extern"};


#define DEFAULT_ICON_THEME		(xfce_icon_theme_get_for_screen(NULL))
#define UNREF(x)							if((x)) {g_object_unref((x));}



typedef struct
{
	GtkWidget *vbox;
	GtkWidget *linebox;
	GtkWidget *label;  //nb_lines
	GtkWidget *spin1;   //nb_lines
	GtkWidget *configbox;
	GtkWidget *scrolledwindow1;
	GtkWidget *treeview1;
	GtkWidget *vbuttonbox1;
	GtkWidget *btn_new;
	GtkWidget *btn_remove;
	GtkWidget *btn_edit;
	GtkWidget *alignment1;
	GtkWidget *hbox2;
	GtkWidget *image1;
	GtkWidget *btn_up;
	GtkWidget *btn_down;
}
t_qck_launcher_opt_dlg;


typedef struct
{  
	GtkWidget *widget;
	gchar *command;
	gchar *icon_name;
	gint icon_id;
	gulong command_id;  
}
t_launcher;


typedef struct
{  
	GList *launchers;
	GList *callback_data;
	GtkWidget *table;
	GtkWidget *base;
	gint icon_size;
	gint orientation;
	gint nb_lines;
	gint nb_launcher;
	gint panel_size; //can't get it another way
}
t_quicklauncher;


//taken from systembuttons.c ;-)
typedef struct
{
    const char *signal;
    GCallback callback;
    gpointer data;
}
SignalCallback; 

//Global var
extern t_quicklauncher *_quicklauncher;

GdkPixbuf *
_create_pixbuf(gint id, const gchar* name, gint size);

t_launcher* launcher_new (const gchar *command, gint icon_id, const gchar *icon_name);

void launcher_free (t_launcher *launcher);

void launcher_update_command(t_launcher *launcher); 

void launcher_update_icon(t_launcher *launcher) ;

void quicklauncher_add_element( t_launcher *launcher);

t_launcher* quicklauncher_remove_element(gint num);

void quicklauncher_organize();

void quicklauncher_empty_widgets();

void quicklauncher_set_nblines(gint nb_lines);

#endif
