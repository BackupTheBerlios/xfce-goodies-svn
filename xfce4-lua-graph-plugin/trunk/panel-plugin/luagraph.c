/*  $Id$
 *  Lua-Driven Graph Applet for XFCE4 panel
 *
 *  Copyright © 2005 A. Gordon <agordon88@gmail.com>
 *
 *  Heavily Based on Megahertz XFCE4 Panel Version 0.1 by:
 *  Copyright Â© 2005 Wit Wilinski <madman@linux.bydg.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <config.h>

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif

#include <stdio.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib.h>

#include <libxfce4util/libxfce4util.h>

#include <panel/global.h>
#include <panel/controls.h>
#include <panel/icons.h>
#include <panel/plugins.h>

#include <lua50/lua.h>
#include <lua50/lauxlib.h>
#include <lua50/lualib.h>

const char *demo_script = 
"function init() \n" \
" add_label(\"lbl\",\"<span foreground=\\\"blue\\\" size=\\\"large\\\">Lua-Graph</span>\\n<i>Demo</i>\")\n" \
" add_progressbar(\"prg1\",65535,0,65535)\n" \
" set_interval(50) \n" \
" idx = 1\n" \
"end \n" \
"\n" \
"function update() \n" \
" idx = idx + 1 \n" \
" if idx>180 then \n" \
"   idx = 0  \n" \
" end \n" \
" local val = math.floor(math.sin(math.rad(idx))*100) \n" \
" set_progress_bar_color(\"prg1\",get_status_color(val)) \n" \
" set_progress_bar_value(\"prg1\",val); \n" \
"end \n" ;

typedef struct
{
	GtkWidget *eventbox;
	GtkWidget *container ;
	GtkWidget *filechooser;
	GtkTooltips *tips;
	
	gchar *script_filename;
	
	int size;
	int interval;
	int timeout_id;
	
	GHashTable* widgets;
	lua_State *lua;
}
XfceLuaGraph;

static gboolean
luagraph_timeout (XfceLuaGraph *ac)
{
	/* 
	   Call a lua function from C
	   See "Programming in Lua", section 25.2
	*/
	lua_getglobal(ac->lua,"update");
	if (lua_pcall(ac->lua,0,0,0) != 0) {
		printf("Error runnung lua function 'update': %s\n",lua_tostring(ac->lua,-1));
		lua_pop(ac->lua, 1);  /* pop error message from the stack */
	}
    return TRUE;
}

static GtkWidget *
markup_label_new(int size)
{
	GtkWidget *lbl;
	lbl = gtk_label_new (NULL);
	gtk_widget_show (lbl);
	gtk_label_set_use_markup (GTK_LABEL (lbl), TRUE);
	return lbl;
}

static GtkWidget *
color_progress_bar_new(int size, GtkProgressBarOrientation orientation, GdkColor *color)
{
	GtkWidget *prg;
	GtkRcStyle *rc ;
	
	prg = gtk_progress_bar_new() ;
	gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(prg),orientation);
	
        rc = gtk_widget_get_modifier_style(prg);
        
        if (!rc) {
		rc = gtk_rc_style_new();
        } else {
		gtk_rc_style_ref(rc);
        }
                
        if (rc) {
            rc->color_flags[GTK_STATE_PRELIGHT] |= GTK_RC_BG;
            rc->color_flags[GTK_STATE_SELECTED] |= GTK_RC_BASE;
            rc->bg[GTK_STATE_PRELIGHT] = *color;
            rc->base[GTK_STATE_SELECTED] = *color;
            
            gtk_widget_modify_style(prg, rc);
            gtk_rc_style_unref(rc);
        }

	gtk_widget_set_size_request(GTK_WIDGET(prg),6 + 2*size,icon_size[size]-4);
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(prg), 0.5) ;
	
	gtk_widget_show(prg);
	
	return prg;
}

void set_label_text(XfceLuaGraph *ac, const gchar* name, const gchar *text)
{
	GtkWidget *ctrl ;

	g_return_if_fail (ac != NULL);
	
	ctrl = g_hash_table_lookup(ac->widgets,name);
	
	g_return_if_fail(ctrl != NULL);
	
	gtk_label_set_markup(GTK_LABEL(ctrl), text);
}

void set_progress_bar_fraction(XfceLuaGraph *ac, const gchar* name, const gdouble frac)
{
	GtkWidget *ctrl ;

	g_return_if_fail (ac != NULL);
	
	ctrl = g_hash_table_lookup(ac->widgets,name);
	
	g_return_if_fail(ctrl != NULL);
	
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ctrl), frac);
}

void set_progress_bar_color(XfceLuaGraph *ac, const gchar* name, const GdkColor *clr)
{
	GtkRcStyle *rc ;
	GtkWidget *ctrl ;

	g_return_if_fail (ac != NULL);
	
	ctrl = g_hash_table_lookup(ac->widgets,name);
	
	g_return_if_fail(ctrl != NULL);

        rc = gtk_widget_get_modifier_style(ctrl);
        
        if (!rc) {
		rc = gtk_rc_style_new();
        } else {
		gtk_rc_style_ref(rc);
        }
                
        if (rc) {
            rc->color_flags[GTK_STATE_PRELIGHT] |= GTK_RC_BG;
            rc->color_flags[GTK_STATE_SELECTED] |= GTK_RC_BASE;
            rc->bg[GTK_STATE_PRELIGHT] = *clr;
            rc->base[GTK_STATE_SELECTED] = *clr;
            
            gtk_widget_modify_style(ctrl, rc);
            gtk_rc_style_unref(rc);
        }
}

/*
	Convert HSV to RGB, but assume S & V values are fixed at maximum.
*/
void H_to_RGB(int H, GdkColor *clr)
{
	const int max_clr = 65535;
	int R,G,B;
	
	if (H>=0 && H<=60) {
		B = 0 ;
		R = max_clr ;
		G = (max_clr * H) / 60 ;
	}
	else
	if (H>60 && H<=120) {
		B = 0 ;
		G = max_clr;
		R = (max_clr*(120-H))/60 ;
	}
	else
	if (H>120 && H<=180) {
		R = 0 ;
		G = max_clr ;
		B = (max_clr*(H-120))/60 ;
	}
	else
	if (H>180 && H<=240) {
		R = 0 ;
		B = max_clr ;
		G = (max_clr*(240-H))/60 ;
	}
	if (H>240 && H<=300) {
		G = 0 ;
		B = max_clr ;
		R = (max_clr*(H-240))/60 ;
	}
	if (H>300 && H<=360) {
		G = 0 ;
		R = max_clr ;
		B = (max_clr*(360-H))/60 ;
	}
	clr->red = R ;
	clr->green = G;
	clr->blue = B ;
}

static int lua_update_get_status_color(lua_State *L) 
{
	GdkColor clr;
	const int l = luaL_checkint(L,1);
	
	H_to_RGB( l*120/100 , &clr);
	
	lua_pushnumber(L,clr.red);
	lua_pushnumber(L,clr.green);
	lua_pushnumber(L,clr.blue);
	return 3;	
}

static const char LuaStructKey = 'k';

void lua_set_data_struct(lua_State *L,const XfceLuaGraph *ac)
{
	lua_pushlightuserdata(L,(void*)&LuaStructKey);
	lua_pushlightuserdata(L,(void*)ac);
	lua_settable(L,LUA_REGISTRYINDEX);
}

XfceLuaGraph* lua_get_data_struct(lua_State *L)
{
	XfceLuaGraph* ac;
	
	lua_pushlightuserdata(L,(void*)&LuaStructKey);
	lua_gettable(L,LUA_REGISTRYINDEX);
	
	ac = (XfceLuaGraph*)lua_touserdata(L,-1) ;
	
	return ac;
}

static char* small_text_size[4] = {
	"x-small",
	"small",
	"small",
	"medium"
};
static char* large_text_size[4] = {
	"large",
	"x-large",
	"x-large",
	"xx-large"
};
static int lua_update_get_small_text_span(lua_State *L) 
{
	XfceLuaGraph* ac;
	ac = lua_get_data_struct(L);
	
	switch(ac->size)
	{
	case 0:
	case 1:
	case 2:
	case 3:
		lua_pushstring(L,small_text_size[ac->size]);
		break;
	
	default:
		lua_pushstring(L,small_text_size[3]);
		break;
	}
	return 1;
}

static int lua_update_get_small_text_string(lua_State *L) 
{
	XfceLuaGraph* ac;
	const char *text = luaL_checkstring(L,1);
	
	ac = lua_get_data_struct(L);
	
	switch(ac->size)
	{
	case 0:
	case 1:
	case 2:
	case 3:
		lua_pushfstring(L,"<span size=\"%s\">%s</span>",small_text_size[ac->size],text);
		break;
	
	default:
		lua_pushfstring(L,"<span size=\"%s\">%s</span>",small_text_size[3],text);
		break;
	}
	return 1;
}

static int lua_update_get_large_text_span(lua_State *L) 
{
	XfceLuaGraph* ac;
	ac = lua_get_data_struct(L);
	
	switch(ac->size)
	{
	case 0:
	case 1:
	case 2:
	case 3:
		lua_pushstring(L,large_text_size[ac->size]);
		break;
	
	default:
		lua_pushstring(L,large_text_size[3]);
		break;
	}
	return 1;
}

static int lua_update_get_large_text_string(lua_State *L) 
{
	XfceLuaGraph* ac;
	const char *text = luaL_checkstring(L,1);
	
	ac = lua_get_data_struct(L);
	
	switch(ac->size)
	{
	case 0:
	case 1:
	case 2:
	case 3:
		lua_pushfstring(L,"<span size=\"%s\">%s</span>",large_text_size[ac->size],text);
		break;
	
	default:
		lua_pushfstring(L,"<span size=\"%s\">%s</span>",large_text_size[3],text);
		break;
	}
	return 1;
}

static int lua_init_set_interval(lua_State *L) 
{
	XfceLuaGraph* ac;
	
	const int interval = luaL_checkint(L,1);
	ac = lua_get_data_struct(L);
	
	ac->interval = interval ;
	
	return 0 ;
}

static int lua_init_create_label (lua_State *L) 
{
	XfceLuaGraph* ac;
	GtkWidget *ctrl;
	GtkWidget *hbox;
	
	const char *ctrl_name = luaL_checkstring(L,1);
	const char *label_text = luaL_checkstring(L,2);

	ac = lua_get_data_struct(L);
	hbox = ac->container;
	
	//printf("Creating label \"%s\" with text = \"%s\"\n", ctrl_name, label_text ) ;
	
	ctrl = markup_label_new(ac->size) ; 
	gtk_box_pack_start(GTK_BOX(hbox),GTK_WIDGET(ctrl),FALSE, FALSE, 0);
	g_hash_table_insert(ac->widgets,(char*)ctrl_name,ctrl);
	set_label_text(ac, ctrl_name,label_text);
	
//	printf("New label(%s), requested = (%d,%d,%d,%d)\n", label_text, GTK_LABEL(ctrl)
	
	return 0;  /* number of results */
}

static int lua_init_create_progress_bar (lua_State *L) 
{
	XfceLuaGraph* ac;
	GtkWidget *ctrl;
	GtkWidget *hbox;
	GdkColor clr;
	
	const char *ctrl_name = luaL_checkstring(L,1);
	const int red = luaL_checkint(L,2);
	const int green = luaL_checkint(L,3);
	const int blue = luaL_checkint(L,4);

	//printf("Creating Progress Bar \"%s\" with color = (%d,%d,%d)\n", ctrl_name, red,green,blue ) ;
	clr.red = red ;
	clr.green = green ;
	clr.blue = blue;
	
	ac = lua_get_data_struct(L);
	hbox = ac->container;

	ctrl = color_progress_bar_new(ac->size,GTK_PROGRESS_BOTTOM_TO_TOP, &clr) ;
	gtk_box_pack_start(GTK_BOX(hbox),GTK_WIDGET(ctrl),FALSE, FALSE, 0);
	g_hash_table_insert(ac->widgets,(char*)ctrl_name,ctrl);
		      	      
	set_progress_bar_fraction(ac, ctrl_name,0.3);
	set_progress_bar_color(ac, ctrl_name,&clr );
	
	return 0;  /* number of results */
}

static int lua_update_set_label_text(lua_State *L) 
{
	XfceLuaGraph* ac;
	
	const char *ctrl_name = luaL_checkstring(L,1);
	const char *label_text = luaL_checkstring(L,2);

	//printf("Updating Label \"%s\" to text = \"%s\"\n", ctrl_name, label_text ) ;

	ac = lua_get_data_struct(L);
	set_label_text(ac,ctrl_name,label_text);
}

static int lua_update_set_progress_bar_fraction (lua_State *L) 
{
	XfceLuaGraph* ac;
	
	const char *ctrl_name = luaL_checkstring(L,1);
	const int frac = luaL_checkint(L,2);

	//printf("Setting Progress Bar \"%s\" to fraction = %3.3f\n", ctrl_name, (double)frac/100) ;
	
	ac = lua_get_data_struct(L);
	set_progress_bar_fraction(ac,ctrl_name,(double)frac/100);
}

static int lua_update_set_progress_bar_color (lua_State *L) 
{
	XfceLuaGraph* ac;
	GdkColor clr;
	
	const char *ctrl_name = luaL_checkstring(L,1);
	const int red = luaL_checkint(L,2);
	const int green = luaL_checkint(L,3);
	const int blue = luaL_checkint(L,4);
	clr.red = red ;
	clr.green = green ;
	clr.blue = blue;

	//printf("Creating Progress Bar \"%s\" with color = (%d,%d,%d)\n", ctrl_name, red,green,blue ) ;

	ac = lua_get_data_struct(L);
	
	set_progress_bar_color(ac,ctrl_name,&clr);
}


void
luagraph_call_lua_initialization(XfceLuaGraph *ac)
{
	
	GdkColor clr;
	int h;
	int error;
	
	g_return_if_fail(ac != NULL);
	
	ac->interval = 1000 ;

	/* initialize LUA */
	ac->lua = lua_open();
	g_return_if_fail(ac->lua != NULL) ;

	luaopen_base(ac->lua);
	luaopen_table(ac->lua);
	luaopen_io(ac->lua);
	luaopen_string(ac->lua);
	luaopen_math(ac->lua);

//	printf("script=\n%s\n\n",demo_script);
	
	/* Load the LUA file (or the built-in demo script) and run it once */
	if (ac->script_filename==NULL) {
		error = luaL_loadbuffer(ac->lua,demo_script,strlen(demo_script),"demo");
	} else {
		error = luaL_loadfile(ac->lua,ac->script_filename);
	}
	if (error) {
          printf("LuaError: %s", lua_tostring(ac->lua, -1));
          lua_pop(ac->lua, 1);  /* pop error message from the stack */
	  g_return_if_fail(0);
	}
	
	g_return_if_fail(lua_pcall(ac->lua,0,0,0) == 0) ;
	
	lua_set_data_struct(ac->lua,ac);
	
	/*
	   Register C function for lua scripts
	   See "Programming in Lua", section 26.1
	*/
	lua_pushcfunction(ac->lua,lua_init_set_interval);
	lua_setglobal(ac->lua,"set_interval");

	lua_pushcfunction(ac->lua,lua_init_create_label);
	lua_setglobal(ac->lua,"add_label");

	lua_pushcfunction(ac->lua,lua_init_create_progress_bar);
	lua_setglobal(ac->lua,"add_progressbar");
	
	/* 
	   Call a lua function from C
	   See "Programming in Lua", section 25.2
	*/
	lua_getglobal(ac->lua,"init");
	if (lua_pcall(ac->lua,0,0,0) != 0) {
		printf("Error runnung lua function 'init': %s\n",lua_tostring(ac->lua,-1));
		lua_pop(ac->lua, 1);  /* pop error message from the stack */
		g_return_if_fail(0);
	}
		
	/*
	    Delete the Initialization C functions
	    (to prevent funky lua scripts from creating more controls, in the 'update' lua functions)
	*/
	lua_pushnil(ac->lua);
	lua_setglobal(ac->lua,"add_label");

	lua_pushnil(ac->lua);
	lua_setglobal(ac->lua,"add_progressbar");

	lua_pushnil(ac->lua);
	lua_setglobal(ac->lua,"set_interval");
	
	/*
	    Register Update C functions
	*/
	lua_pushcfunction(ac->lua,lua_update_set_label_text);
	lua_setglobal(ac->lua,"set_label_text");

	lua_pushcfunction(ac->lua,lua_update_set_progress_bar_fraction);
	lua_setglobal(ac->lua,"set_progress_bar_value");

	lua_pushcfunction(ac->lua,lua_update_set_progress_bar_color);
	lua_setglobal(ac->lua,"set_progress_bar_color");

	lua_pushcfunction(ac->lua,lua_update_get_large_text_string);
	lua_setglobal(ac->lua,"large_text");

	lua_pushcfunction(ac->lua,lua_update_get_small_text_string);
	lua_setglobal(ac->lua,"small_text");
	
	lua_pushcfunction(ac->lua,lua_update_get_small_text_span);
	lua_setglobal(ac->lua,"small_text_span");
	
	lua_pushcfunction(ac->lua,lua_update_get_large_text_span);
	lua_setglobal(ac->lua,"large_text_span");
	
	lua_pushcfunction(ac->lua,lua_update_get_status_color);
	lua_setglobal(ac->lua,"get_status_color");

	ac->timeout_id =
		g_timeout_add (ac->interval, (GSourceFunc) luagraph_timeout, ac);
}

static gboolean destroy_xfce_panel_widget(gpointer key, gpointer value, gpointer data)
{
	int width ;
	XfceLuaGraph *ac = (XfceLuaGraph*)data;
	g_return_if_fail(ac != NULL);
	
	gtk_widget_destroy(value);
	
	return TRUE;
}

void
luagraph_lua_cleanup(XfceLuaGraph *ac)
{
	
	GdkColor clr;
	int h;
	int error;
	
	g_return_if_fail(ac != NULL);
	
	/* Close the LUA state machine */
	if (ac->lua) {
		lua_close(ac->lua);
		ac->lua = NULL ;
	}
	
	/* Destry all the widgets, and clear the widget hash table */
	g_hash_table_foreach_remove(ac->widgets,destroy_xfce_panel_widget,ac);
	
	/* disable the update timeout */
	    if (ac->timeout_id)
	    {
		g_source_remove (ac->timeout_id);
		ac->timeout_id = 0;
	    }
}

static XfceLuaGraph *
luagraph_new (void)
{
	XfceLuaGraph *ac = g_new (XfceLuaGraph, 1);
	GtkWidget *hbox;
	GtkWidget *ctrl;
	
	ac->script_filename = NULL ;
	ac->interval = 1000 ;
	ac->size = settings.size;
	ac->widgets = g_hash_table_new(g_str_hash,g_str_equal) ;
	
	/* Tool tip widget */
	ac->tips = gtk_tooltips_new ();
	g_object_ref (ac->tips);
	gtk_object_sink (GTK_OBJECT (ac->tips));
	
	/* Base EventBox & HBox Container */
	ac->eventbox = gtk_event_box_new ();
	gtk_widget_set_name (ac->eventbox, "xfce_panel");
	gtk_widget_show (ac->eventbox);
	
	hbox = GTK_WIDGET(gtk_hbox_new(FALSE, 0));
	ac->container = hbox ;
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
	gtk_widget_show (hbox);
	gtk_container_add (GTK_CONTAINER (ac->eventbox), hbox);

	/* Load the LUA state machine, and the script file */
	luagraph_call_lua_initialization(ac);
	
	luagraph_timeout (ac);
	
	return ac;
}

static void
luagraph_free (Control * control)
{
	XfceLuaGraph *ac = control->data;
	
	g_return_if_fail (ac != NULL);
	
	g_object_unref (ac->tips);
	
	luagraph_lua_cleanup(ac);
	
	g_free (ac);
}

static void
luagraph_attach_callback (Control * control, const char *signal,
		       GCallback callback, gpointer data)
{
	XfceLuaGraph *ac = control->data;
	g_signal_connect (ac->eventbox, signal, callback, data);
}

static void set_widget_xfce_size(gpointer key, gpointer value, gpointer data)
{
	int width ;
	XfceLuaGraph *ac = (XfceLuaGraph*)data;
	g_return_if_fail(ac != NULL);
	
	if (GTK_IS_LABEL(value)) {
		width = -1; 
	} else {
		width = 6 + 2*ac->size;
	}
	gtk_widget_set_size_request(GTK_WIDGET(value),width,icon_size[ac->size]-4);
}

/* panel preferences */
static void
luagraph_set_size (Control * control, int size)
{
	XfceLuaGraph *ac = control->data;
	
	ac->size = size;
	
	gtk_widget_set_size_request (ac->eventbox, -1, -1);
	
	g_hash_table_foreach(ac->widgets,set_widget_xfce_size,ac);
	
	luagraph_timeout (ac);
}

static gboolean
create_luagraph_control (Control * control)
{
	XfceLuaGraph *ac = luagraph_new ();
	
	gtk_container_add (GTK_CONTAINER (control->base), ac->eventbox);
	control->data = (gpointer) ac;
	control->with_popup = FALSE;
	gtk_widget_set_size_request (control->base, -1, -1);
	
	return TRUE;
}



static void
luagraph_read_config(Control *ctrl, xmlNodePtr parent)
{
	XfceLuaGraph *ac = (XfceLuaGraph *)ctrl->data;
	xmlNodePtr node;
	xmlChar *value;

	if (parent == NULL || parent->children == NULL)
		return;
	
	luagraph_lua_cleanup(ac);
	
	for (node = parent->children; node != NULL; node = node->next) {
		if (!xmlStrEqual(node->name, (const xmlChar*)"LuaGraph"))
			continue;
	
		if ((value = xmlGetProp(node, (const xmlChar*)"script")) != NULL) {
			ac->script_filename = g_strdup((const char *)value);
			xmlFree(value);
		}
		break;
	}
	
	luagraph_call_lua_initialization(ac);
}

static void
luagraph_write_config(Control *ctrl, xmlNodePtr parent)
{
	XfceLuaGraph *ac = (XfceLuaGraph *)ctrl->data;
	xmlNodePtr root;
	
	root = xmlNewTextChild(parent, NULL, (const xmlChar*)"LuaGraph", NULL);
	if (ac->script_filename != NULL) {
		xmlSetProp(root, (const xmlChar*)"script", (const xmlChar*)ac->script_filename);
	}
}

static void luagraph_apply_options(GtkWidget *button, XfceLuaGraph* ac)
{
	luagraph_lua_cleanup(ac);
	
	if (ac->script_filename!=NULL) {
		g_free(ac->script_filename);
	}
	ac->script_filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(ac->filechooser)) ;
	printf("Selected File = %s\n", ac->script_filename);
	
	luagraph_call_lua_initialization(ac);
}

/* options dialog */
static void
luagraph_create_options (Control *ctrl, GtkContainer *con, GtkWidget *done)
{
	XfceLuaGraph *ac = (XfceLuaGraph *)ctrl->data;
	
	GtkWidget *hbox, *label, *interface, *vbox, *autohide;
	GtkWidget *filechoose;
	
	vbox = gtk_vbox_new(FALSE, 2);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(con), vbox);
	
	hbox = gtk_hbox_new(FALSE, 2);
	gtk_widget_show(hbox);
	label = gtk_label_new(_("Lua Script:"));
	gtk_widget_show(label);
	
	filechoose = gtk_file_chooser_button_new (_("Select Lua Script"), GTK_FILE_CHOOSER_ACTION_OPEN);
	ac->filechooser = filechoose;
	
	if (ac->script_filename != NULL) {
		gtk_file_chooser_select_filename(GTK_FILE_CHOOSER(filechoose),ac->script_filename);
	} else {
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(filechoose),getenv("HOME"));
	}
	gtk_widget_set_size_request(GTK_WIDGET(filechoose),200,-1) ;
	gtk_widget_show (filechoose);
	
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(hbox), filechoose, TRUE, TRUE, 1);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 1);
	
	g_signal_connect(GTK_WIDGET(done), "clicked",
		G_CALLBACK(luagraph_apply_options), ac);
}

G_MODULE_EXPORT void
xfce_control_class_init (ControlClass * cc)
{
	xfce_textdomain (GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");
	
	cc->name = "luagraph";
	cc->caption = _("Lua-Graph");
	
	cc->create_control = (CreateControlFunc) create_luagraph_control;
	
	cc->free = luagraph_free;
	cc->attach_callback = luagraph_attach_callback;
	cc->set_size = luagraph_set_size;
	
	cc->create_options = luagraph_create_options;
	cc->read_config    = luagraph_read_config;
	cc->write_config   = luagraph_write_config;
	
	control_class_set_unique (cc, FALSE);
}

/* macro defined in plugins.h */
XFCE_PLUGIN_CHECK_INIT
