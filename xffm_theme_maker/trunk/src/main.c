/*
 * C 2003 Edscott Wilson Garcia under GPL
 * This software is bound by the GPL, see accompanying files for
 * more information.
 *
 * <edscott@users.sourceforge.net>
 *
 *  */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <unistd.h>
#include <gtk/gtk.h>
#include <string.h>
#include <gmodule.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "main_gui.h"
#include "support.h"
#include "callbacks.h"

#warning "Please note that xffm_theme_maker requires xffm >= 4.1"

GtkTreeView *treeview;
GtkTreeStore *store;  
GtkWidget *xffm_theme_maker;

enum
{
    DND_NONE,
    DND_MOVE,
    DND_COPY,
    DND_LINK
};

enum
{
    TARGET_URI_LIST,
    TARGET_PLAIN,
    TARGET_STRING,
    TARGETS
};


static GtkTargetEntry target_table[] = {
    {"text/uri-list", 0, TARGET_URI_LIST},
    {"text/plain", 0, TARGET_PLAIN},
    {"STRING", 0, TARGET_STRING}
};
#define NUM_TARGETS (sizeof(target_table)/sizeof(GtkTargetEntry))

#ifndef __MIME_H__
#define __MIME_H__
static GModule *xfmime_icon_cm=NULL;

typedef struct _xfmime_icon_functions xfmime_icon_functions;
struct _xfmime_icon_functions {
   GtkIconSet *(*mime_icon_get_iconset)(const gchar *id, GtkWidget *main_window, const gchar *theme);
   void (*mime_icon_add_iconset)(const gchar *tag, GtkIconSet *icon_set);
   GHashTable *(*mime_icon_load_theme)(const gchar *theme);
   GtkWidget *(*mime_icon_create_pixmap) (GtkWidget *widget,const gchar *filename);
   GdkPixbuf *(*mime_icon_create_pixbuf) (const gchar *filename);
   gchar *(*mime_icon_find_pixmap_file)(const gchar     *filename);
};


#endif
static GtkStyle *style;

xfmime_icon_functions *xfmime_icon_fun=NULL;

char *theme;
    

#define MIME_ICON_get_iconset (*(load_mime_icon_module()->mime_icon_get_iconset))
#define MIME_ICON_add_iconset (*(load_mime_icon_module()->mime_icon_add_iconset))
#define MIME_ICON_load_theme (*(load_mime_icon_module()->mime_icon_load_theme))
#define MIME_ICON_create_pixmap (*(load_mime_icon_module()->mime_icon_create_pixmap))
#define MIME_ICON_create_pixbuf (*(load_mime_icon_module()->mime_icon_create_pixbuf))
#define MIME_ICON_find_pixmap_file (*(load_mime_icon_module()->mime_icon_find_pixmap_file))



void unload_mime_icon_module(void){
	if (!xfmime_icon_fun) return;
	g_free(xfmime_icon_fun);
	xfmime_icon_fun = NULL;
	

	if (!g_module_close(xfmime_icon_cm)){
	   g_warning("g_module_close(xfmime_icon_cm) != TRUE\n");
	}
#ifdef DEBUG
	else {
           g_message ("xffm: module libxffm_mime_icons unloaded");
	}	   
#endif
	xfmime_icon_cm=NULL;
}

xfmime_icon_functions *load_mime_icon_module(void){
    gchar *library, *module;
    xfmime_icon_functions *(*module_init)(void) ;
    
    if (xfmime_icon_fun) return xfmime_icon_fun;
    
    library=g_strconcat("libxffm_mime_icons.",G_MODULE_SUFFIX, NULL);
    module = g_build_filename (LIBDIR, "xffm", library, NULL);
    
    xfmime_icon_cm=g_module_open (module, 0);
    if (!xfmime_icon_cm){
	g_error("g_module_open(%s) == NULL\n",module);
   	exit(1);
    }
    
   if (!g_module_symbol (xfmime_icon_cm, "module_init",(gpointer *) &(module_init)) ) {
	g_error("g_module_symbol(module_init) != FALSE\n");
        exit(1);
    }

    
    xfmime_icon_fun = (*module_init)();
    
           
    /* these functions are imported from the module */
    if (!g_module_symbol (xfmime_icon_cm, "mime_icon_get_iconset",(gpointer *) &(xfmime_icon_fun->mime_icon_get_iconset)) ||    
	!g_module_symbol (xfmime_icon_cm, "mime_icon_add_iconset",(gpointer *) &(xfmime_icon_fun->mime_icon_add_iconset)) ||    
	!g_module_symbol (xfmime_icon_cm, "mime_icon_load_theme",(gpointer *) &(xfmime_icon_fun->mime_icon_load_theme)) ||    
	!g_module_symbol (xfmime_icon_cm, "mime_icon_find_pixmap_file",(gpointer *) &(xfmime_icon_fun->mime_icon_find_pixmap_file)) ||    
	!g_module_symbol (xfmime_icon_cm, "mime_icon_create_pixmap",(gpointer *) &(xfmime_icon_fun->mime_icon_create_pixmap)) ||    
	!g_module_symbol (xfmime_icon_cm, "mime_icon_create_pixbuf",(gpointer *) &(xfmime_icon_fun->mime_icon_create_pixbuf))    
       ) {
	g_error("g_module_symbol() != FALSE\n");
        exit(1);
    }
#ifdef DEBUG
    g_message ("xffm: module %s successfully loaded", library);	    
#endif
    g_free(library);
    g_free(module);
    return xfmime_icon_fun;
}

static char *dnd_data = NULL;

void
on_treeview1_drag_data_get             (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        GtkSelectionData *selection_data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data)
{
    /*printf("drag get\n");*/
    GtkTreeView *treeview = (GtkTreeView *)widget;
    GtkTreeIter iter;
    GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);
    gchar *n;
    
    if (dnd_data) g_free(dnd_data);
    gtk_tree_selection_get_selected (selection,(GtkTreeModel **)(&store),&iter);
                     /* GtkTreeModel **model,GtkTreeIter *iter);*/
    gtk_tree_model_get((GtkTreeModel *)store, &iter, ICON_COLUMN, &n, -1);
    if (n) {
      gchar *i=MIME_ICON_find_pixmap_file(n);
      if (i){
        dnd_data = g_strdup(i);
        gtk_selection_data_set(selection_data, selection_data->target, 
		    8, (const guchar *)dnd_data,strlen(dnd_data)+ 1); 
	g_free(i);
      }
    }
    
    return;

}

xmlNodePtr rootM;
static gboolean xmladd(GtkTreeModel * treemodel, GtkTreePath * treepath, GtkTreeIter * iter, gpointer data){
	gchar *n,*i;
    xmlNodePtr node;
    gtk_tree_model_get(treemodel, iter, NAME_COLUMN, &n, -1);
    gtk_tree_model_get(treemodel, iter, ICON_COLUMN, &i, -1);
    if (i && strlen(i)) {
        node = xmlNewTextChild(rootM, NULL, "mime-type", NULL);
    	xmlSetProp(node,"type", n);
    	xmlSetProp(node,"icon",i);
    }

    if (n) g_free(n);
    if (i) g_free(i);
    return FALSE;
}


static void writexml(void){
    xmlDocPtr doc;
    char *mimefile=NULL;
    
    mimefile=g_strconcat("mime.xml",NULL);
    
    
    {
       doc = xmlNewDoc("1.0");
       doc->children = xmlNewDocRawNode(doc, NULL, "mime-info", NULL);

       rootM = (xmlNodePtr) doc->children;
       xmlDocSetRootElement(doc, rootM);
    }   
    /*xfceNS = xmlNewNs (root,"http://www.xfce.org/","xfce");
    if (!xfceNS) g_assert_not_reached();*/

    
   
    /* write xml out */
    gtk_tree_model_foreach((GtkTreeModel *)store, xmladd,NULL);
    
    xmlSaveFormatFile(mimefile, doc, 1);

    xmlFreeDoc(doc);
    g_free(mimefile);
    { 
 		GtkWidget *dialog = gtk_message_dialog_new ((GtkWindow *)xffm_theme_maker,
                                  GTK_DIALOG_DESTROY_WITH_PARENT,
                                  GTK_MESSAGE_INFO,
                                  GTK_BUTTONS_CLOSE,"file written as mime.xml\n");
 		gtk_dialog_run (GTK_DIALOG (dialog));
 		gtk_widget_destroy (dialog);
    }

    
}

void
on_save_clicked                        (GtkButton       *button,
                                        gpointer         user_data)
{
	writexml();
}


void
on_quit_clicked                        (GtkButton       *button,
                                        gpointer         user_data)
{
  gtk_main_quit();
}


gboolean on_drag_motion(GtkWidget * widget, GdkDragContext * dc, gint x, gint y, guint t, gpointer data)
{
    GdkDragAction action;

    /*printf("DBG:  on_drag_motion \n"); */



    /* Insert code to get our default action here. */
	action = GDK_ACTION_COPY;




	gdk_drag_status(dc, GDK_ACTION_COPY, t);
    /*fprintf(stderr,"dbg: drag motion done...\n"); */



    return (TRUE);
}


void on_drag_data(GtkWidget * widget, GdkDragContext * context, gint x, gint y, GtkSelectionData * data, guint info, guint time, void *client)
{
    int nitems, action;
    gchar *g,*b
	    ;
    int title_offset;
    GtkTreePath *treepath;
    GtkTreeIter iter;
    GdkPixbuf *icon=NULL;   
    
    /*printf("DBG: on drag data\n"); */

    if(!widget || data->length < 0 || data->format != 8)
    {
	gtk_drag_finish(context, FALSE, FALSE, time);
	return;
    }

    /*printf("dbg:on_drag_data\n"); */


    if(!(info == TARGET_STRING) && !(info == TARGET_URI_LIST))
    {
	gtk_drag_finish(context, FALSE, FALSE, time);
	return;
    }

    if (gtk_tree_view_get_headers_visible(treeview)){
	/* FIXME:    offset to title button assumed at fontheight+8 but should be
	 *           widget vertical ascent, queried from the gtkcontainer child 
	 *           (how do you do that?)
	 *           You can access private treeview information, which is not "legitimate".
	 *           */
  	PangoRectangle logical_rect;
  	PangoLayout *layout = gtk_widget_create_pango_layout((GtkWidget *)treeview,"W");
  	pango_layout_get_pixel_extents(layout, NULL, &logical_rect);
  	title_offset= PANGO_ASCENT(logical_rect) + PANGO_DESCENT(logical_rect);
  	g_object_unref(layout);
	title_offset +=8;
    } 
    
    if(!gtk_tree_view_get_path_at_pos(treeview, x,  y - title_offset, &treepath, NULL, NULL, NULL))
    {
	gtk_drag_finish(context, FALSE, FALSE, time);
	return ;
    }
    gtk_tree_model_get_iter((GtkTreeModel *)store, &iter, treepath);
    gtk_tree_path_free(treepath);


    
    printf("DBG:drag data=%s\n",(const char *) data->data); 
    if (!strstr((const char *) data->data,"file:")) 
    {
	gtk_drag_finish(context, FALSE, FALSE, time);
	return;
    }
    g=g_strdup((const char *) data->data);
    if (strstr(g,"\n")) *(strstr(g,"\n"))=0;
    if (strstr(g,"\r")) *(strstr(g,"\r"))=0;
    b=g_path_get_basename(g);
  /*
    {
      gtk_tree_model_get((GtkTreeModel *)store, &iter, NAME_COLUMN,&n, -1);
      printf("DBG:drag data=%s->%s\n",n,b); 
      g_free(n);
      }
      */
    

    
    icon = gtk_image_get_pixbuf((GtkImage *) MIME_ICON_create_pixmap(xffm_theme_maker, b));
    if (icon) {
	    gtk_tree_store_set(store, &iter, PIXBUF_COLUMN, icon,-1);
	    gtk_tree_store_set(store, &iter, ICON_COLUMN, b,-1);
    } else {
 	GtkWidget *dialog = gtk_message_dialog_new ((GtkWindow *)xffm_theme_maker,
                               GTK_DIALOG_DESTROY_WITH_PARENT,
                               GTK_MESSAGE_ERROR,
                               GTK_BUTTONS_CLOSE,"Cannot render icon from %s\n",b);
 	gtk_dialog_run (GTK_DIALOG (dialog));
 	gtk_widget_destroy (dialog);
    }

    
    
  drag_over:
    g_free(b);
    g_free(g);
    gtk_drag_finish(context, TRUE, FALSE, time);
}



typedef struct column_info_t
{
    int id;
    GType type;
}
column_info_t;

GtkTreeStore *create_treestore(void)
{
    column_info_t column_info[] = {

	{PIXBUF_COLUMN, GDK_TYPE_PIXBUF},
	{GROUP_COLUMN, G_TYPE_STRING},
	{NAME_COLUMN, G_TYPE_STRING},
	{ICON_COLUMN, G_TYPE_STRING}
    };
    return (gtk_tree_store_new(TREE_COLUMNS, 
		column_info[PIXBUF_COLUMN].type, 
		column_info[GROUP_COLUMN].type, 
		column_info[ICON_COLUMN].type,
		column_info[NAME_COLUMN].type));
}

gboolean group_found;
gboolean name_found;
gchar *icon_value;
static gboolean find_row(GtkTreeModel * treemodel, GtkTreePath * treepath, GtkTreeIter * iter, gpointer data)
{
    char *name = g_strdup  ((const gchar *) data);
    char *n;
    GtkIconSet *icon_set=NULL;
    GdkPixbuf *icon=NULL;   

    if (name_found) return TRUE;
    
    gtk_tree_model_get(treemodel, iter, NAME_COLUMN, &n, -1);

    if (n &&  strcmp(n,name)==0){ 
	    
	    /*printf("duplicate %s==%s, looking for icon %s\n",n,name,id);*/
	    name_found=TRUE;
	    if (icon_value) {
		    if (strncmp(icon_value,"gtk-",strlen("gtk-"))==0) 
			    icon=gtk_widget_render_icon(xffm_theme_maker, icon_value, GTK_ICON_SIZE_DIALOG, NULL);
	            else 
			    icon = gtk_image_get_pixbuf((GtkImage *) MIME_ICON_create_pixmap(xffm_theme_maker, icon_value));
		    if (icon) {
			    gtk_tree_store_set((GtkTreeStore *) store, iter, PIXBUF_COLUMN, icon,-1);
   			    g_object_unref (G_OBJECT (icon));
		    }
		    gtk_tree_store_set((GtkTreeStore *) store, iter, ICON_COLUMN, icon_value,-1);
	    }
    } 
    
    g_free(n);
    
    g_free(name);
    return FALSE;
}


#if 0
void destroy_tree(GtkTreeModel * treemodel)
{
    gtk_tree_model_foreach(treemodel, destroy_node_entry, NULL);
    /* tree_details cleanup */
    gtk_tree_store_clear((GtkTreeStore *) treemodel);
}
#endif

/*
void func (gpointer key, gpointer value, gpointer user_data){
	char *t=((value)?(char *)value:"null");
	printf("key=%s value=%s\n",(char *)key,t);
}*/

gboolean   unref_row    (GtkTreeModel *model,
                                             GtkTreePath *path,
                                             GtkTreeIter *iter,
                                             gpointer data){
     gtk_tree_model_unref_node (model,iter);
}

static gboolean find_type(GtkTreeModel * treemodel, GtkTreePath * treepath, GtkTreeIter * iter, gpointer data)
{
    char *name = g_strdup  ((const gchar *) data);
    char *n,*g,*group;

    if (name_found) return TRUE;
    if (!strstr(name,"/")){
	    g_free(name);
	    return FALSE;
    }
    group = g_strdup(name);
    *(strchr(group,'/')) = 0;   /* group = strtok(group,"/");*/
    gtk_tree_model_get(treemodel, iter, GROUP_COLUMN, &g, -1);
    gtk_tree_model_get(treemodel, iter, NAME_COLUMN, &n, -1);
    if (g && strcmp(g,group)==0){
	    GtkTreeIter child;
	    group_found=TRUE;
	    if (!name_found && strcmp(n,name)!=0) {
	      gtk_tree_store_insert (store, &child, iter, 0);
	      gtk_tree_store_set((GtkTreeStore *) store, &child, NAME_COLUMN, name,-1);
	      name_found=TRUE;
	    }
    }
    if (g) g_free(g);
    
    if (n) g_free(n);
    g_free(name);
    g_free(group);
    return FALSE;
}
	    


static gboolean create_icon_tree(const gchar *in_theme){
    gchar *mimefile;
    gchar *typesfile;
    xmlDocPtr doc;
    xmlChar *id, *value;
    xmlNodePtr node;
    static gboolean quit=FALSE;
    int i;

    MIME_ICON_load_theme(in_theme);
    theme=(char *)in_theme;

    if (store){
    	gtk_tree_model_foreach((GtkTreeModel *)store, unref_row, NULL);
	gtk_tree_store_clear (store);
    } else store=create_treestore();

 
    if (!theme) theme="gnome";	    

    printf("DBG: creating icons for theme=%s\n",theme);
   
    mimefile=g_strconcat(PACKAGE_DATA_DIR,
		    G_DIR_SEPARATOR_S,"xffm",
		    G_DIR_SEPARATOR_S,"pixmaps",
		    G_DIR_SEPARATOR_S,theme,
		    G_DIR_SEPARATOR_S,"mime.xml",NULL);

    
    if (access(mimefile,F_OK)!=0){
	/* fall back to plain */
	g_warning("%s theme not found. Using plain icons instead. Install package xffm-icons for richer icons.",
			theme);
	g_free(mimefile);
    	mimefile=g_strconcat(PACKAGE_DATA_DIR,
		    G_DIR_SEPARATOR_S,"xffm",
		    G_DIR_SEPARATOR_S,"pixmaps",
		    G_DIR_SEPARATOR_S,"plain",
		    G_DIR_SEPARATOR_S,"mime.xml",NULL);	    
    } 
    if (access(mimefile,F_OK)!=0) goto error_xml;    

    /*********************** types defined in freedesktop.xml and xfce.xml ***/
    for (i=0;i<2;i++){
	gchar *tfiles[2]={"freedesktop.org.xml", "xfce.org.xml"};

	typesfile=g_strconcat(PACKAGE_DATA_DIR,
		    G_DIR_SEPARATOR_S,"xffm",
		    G_DIR_SEPARATOR_S,tfiles[i],NULL);

	
    	if (access(typesfile,F_OK)!=0) continue;    
    	xmlKeepBlanksDefault(0);
    	if((doc = xmlParseFile(typesfile)) == NULL)
    	{
		g_warning("Error at %s.", typesfile);
		continue;
	}
    	node = xmlDocGetRootElement(doc);
    	if(!xmlStrEqual(node->name, (const xmlChar *)"mime-info"))
    	{
        	xmlFreeDoc(doc);
		continue;
    	}
    	for(node = node->children; node; node = node->next)
    	{
		if(xmlStrEqual(node->name, (const xmlChar *)"mime-type"))
		{
	   	 id = xmlGetProp(node, (const xmlChar *)"type");
		 if (id && strchr(id,'/')){
			GtkTreeIter iter;
			GtkTreeIter child;
			name_found=group_found=FALSE;
			gtk_tree_model_foreach((GtkTreeModel *)store, find_type,id);
			if (!group_found){
				gchar *n,*g=strdup(id);
				*(strchr(g,'/')) = 0; /*g=strtok(g,"/");*/
				n=g_strconcat(g,"/default",NULL);
		  		gtk_tree_store_insert (store, &iter, NULL,0);
		  		gtk_tree_store_set((GtkTreeStore *) store, &iter, GROUP_COLUMN, g,-1);
				gtk_tree_store_set((GtkTreeStore *) store, &iter, NAME_COLUMN, n,-1);
				g_free(n);
				g_free(g);
		  		gtk_tree_store_insert (store, &child, &iter, 0);
				gtk_tree_store_set((GtkTreeStore *) store, &child, NAME_COLUMN, id,-1);
			}
		 	g_free(id);
		 }
		}
	}
    	xmlFreeDoc(doc); 
	g_free(typesfile);
    }
    
    /********************   icons defined in mime.types ***************/
    xmlKeepBlanksDefault(0);
    
    if((doc = xmlParseFile(mimefile)) == NULL)
    {
      error_xml:
	g_warning("No valid mime.xml found for theme.\nIt should be at %s.", mimefile);
	return FALSE;
	goto time2return;
    }
    node = xmlDocGetRootElement(doc);
    if(!xmlStrEqual(node->name, (const xmlChar *)"mime-info"))
    {
        xmlFreeDoc(doc);
	goto error_xml;
    }

    /* Now parse the xml tree */
#ifdef DEBUG
            printf("DBG: parsing %s\n",mimefile);
#endif
    for(node = node->children; node; node = node->next)
    {
	if(xmlStrEqual(node->name, (const xmlChar *)"mime-type"))
	{
	    id = xmlGetProp(node, (const xmlChar *)"type");
	    icon_value = xmlGetProp(node, (const xmlChar *)"icon");
	    
	    if (id && icon_value) {
		name_found=FALSE;
		if(!style) style = gtk_style_new();
		gtk_tree_model_foreach((GtkTreeModel *)store, find_row,id);
		if (!name_found){
			g_warning("type %s not found",id);
		}
	

	    }
	    if (icon_value) g_free(icon_value); 
	    if (id) g_free(id); 
	}
    }

    xmlFreeDoc(doc);
    
time2return:
    g_free(mimefile);
    return TRUE;
}

gint changer;
GtkWidget *dialog;
static gint change_theme(gpointer data){
	GtkMenu			*menu;
	GtkLabel		*label;
	gchar			*txt;
	GtkOptionMenu *om=(GtkOptionMenu *)data;
	g_source_remove(changer);
	/*this makes a memory leak worthy of the Titanic: unload_mime_icon_module();*/
	label = GTK_LABEL (gtk_bin_get_child(GTK_BIN(om)));
	txt = g_strdup (gtk_label_get_text (label));
  	create_icon_tree(txt);
	g_free(txt);
	gtk_main_quit();
	gtk_widget_destroy(dialog);
    
	return FALSE;
}

void theme_changed (GtkOptionMenu *om, gpointer user_data){
	const gchar *message_format="Please wait. Icon regeneration in progress...";
	dialog = gtk_message_dialog_new ((GtkWindow *)xffm_theme_maker,GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
                                             GTK_MESSAGE_INFO,
                                             GTK_BUTTONS_NONE,
					     message_format);
	gtk_window_set_transient_for (GTK_WINDOW (xffm_theme_maker),  GTK_WINDOW (dialog));
        changer=g_timeout_add_full(0, 260, (GtkFunction) change_theme, (gpointer)(om), NULL);
	gtk_widget_show_all(dialog);
	gtk_main();
}

static gboolean icon_changed(GtkCellRendererText *cell,
	     const gchar         *path_string,
	     const gchar         *new_text,
	     gpointer             data)
{             
    GtkTreeModel *model = (GtkTreeModel *)store;
    GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
    GtkTreeIter iter;
    gchar *text;
    GdkPixbuf *icon=NULL;   

    gint *column;

    column = g_object_get_data (G_OBJECT (cell), "column");

    gtk_tree_model_get_iter (model, &iter, path);

    if (GPOINTER_TO_INT (column) == ICON_COLUMN) {
	gchar *old_text;
        gtk_tree_model_get (model, &iter, column, &old_text, -1);
	g_free (old_text);
	if (!new_text) gtk_tree_store_set (store, &iter, column,"",-1);
	else gtk_tree_store_set (store, &iter, column,new_text,-1);
	icon = gtk_image_get_pixbuf((GtkImage *) MIME_ICON_create_pixmap(xffm_theme_maker,new_text ));
	gtk_tree_store_set(store, &iter, PIXBUF_COLUMN, icon,-1);
      	/*if (!icon){ 
 		GtkWidget *dialog = gtk_message_dialog_new ((GtkWindow *)xffm_theme_maker,
                                  GTK_DIALOG_DESTROY_WITH_PARENT,
                                  GTK_MESSAGE_ERROR,
                                  GTK_BUTTONS_CLOSE,"Cannot render icon from %s\n",new_text);
 		gtk_dialog_run (GTK_DIALOG (dialog));
 		gtk_widget_destroy (dialog);
	}*/


    }
    gtk_tree_path_free (path);
    
    return FALSE;
}


int
main (int argc, char *argv[])
{
  GtkCellRenderer *cell;
  GtkTreeViewColumn *column;
  GHashTable *hash_table;
  gtk_set_locale ();
  gtk_init (&argc, &argv); 

  
  xffm_theme_maker = create_xffm_theme_maker ();

  /* themes */
  {
    GDir *gdir;
    const char *file;
    GtkOptionMenu *optionmenu=(GtkOptionMenu *)lookup_widget(xffm_theme_maker,"optionmenu2");
    gchar *theme_dir=g_strconcat(PACKAGE_DATA_DIR, G_DIR_SEPARATOR_S,"xffm", G_DIR_SEPARATOR_S,"pixmaps",G_DIR_SEPARATOR_S,theme,NULL);
    

    {
	gdir = g_dir_open(theme_dir, 0, NULL);
	GtkMenu	*menu=GTK_MENU (gtk_menu_new ());
	
	if(gdir) {
	    while((file = g_dir_read_name(gdir))) {
		char *path = g_build_filename(theme_dir, file, NULL);
		GtkWidget		*it;
		if(g_file_test(path, G_FILE_TEST_IS_DIR)) {
		  it = gtk_menu_item_new_with_label ((gchar *)file);
		  gtk_menu_shell_append (GTK_MENU_SHELL (menu), it);
		  gtk_widget_show (it);
			
		}
		g_free(path);
	    }
	    g_dir_close(gdir);
	}
	gtk_option_menu_set_menu (optionmenu, GTK_WIDGET (menu));
    }
    g_free(theme_dir);
    /* populate the model: */
    {
	GtkLabel		*label;
	gchar			*txt;
	label = GTK_LABEL (gtk_bin_get_child(GTK_BIN(optionmenu)));
	txt = g_strdup (gtk_label_get_text (label));
        create_icon_tree(txt);
	g_free(txt);
    }
    g_signal_connect(G_OBJECT(optionmenu), "changed", G_CALLBACK(theme_changed), NULL);
  }
  

  /* treeview */
  
  treeview =(GtkTreeView *)lookup_widget((GtkWidget *)xffm_theme_maker,"treeview1");
  gtk_tree_view_set_model (treeview,(GtkTreeModel *)store);
   

  gtk_drag_dest_set((GtkWidget *) treeview, GTK_DEST_DEFAULT_DROP | GTK_DEST_DEFAULT_HIGHLIGHT, target_table, NUM_TARGETS, GDK_ACTION_MOVE | GDK_ACTION_COPY | GDK_ACTION_LINK);

  gtk_drag_source_set((GtkWidget *) treeview, GDK_BUTTON1_MASK | GDK_BUTTON2_MASK, target_table, NUM_TARGETS, GDK_ACTION_MOVE | GDK_ACTION_COPY | GDK_ACTION_LINK);



    /* create the pixbuf column */
    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_resizable(column, FALSE);
    gtk_tree_view_column_set_reorderable(column, FALSE);
    gtk_tree_view_column_set_spacing(column,2);

    cell = gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start(column, cell, FALSE);
    gtk_tree_view_column_set_attributes(column, cell, 
		    "pixbuf", PIXBUF_COLUMN, 
		    "pixbuf_expander_closed", PIXBUF_COLUMN, 
		    "pixbuf_expander_open", PIXBUF_COLUMN, 
		    NULL);


     gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
    gtk_tree_view_set_expander_column(treeview, column);

    /* group */ 

    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_resizable(column, FALSE);
    gtk_tree_view_column_set_reorderable(column, FALSE);
    gtk_tree_view_column_set_spacing(column,2);
    cell = gtk_cell_renderer_text_new();
    g_object_set(G_OBJECT(cell), "editable", FALSE, NULL);
    gtk_tree_view_column_set_title(column, "Group");
    gtk_tree_view_column_set_clickable(column, TRUE);

    gtk_tree_view_column_pack_start(column, cell, FALSE);
    gtk_tree_view_column_set_attributes(column, cell, 
		    "text", GROUP_COLUMN, 
		    NULL);
     gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

     /* name */
     column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_resizable(column, FALSE);
    gtk_tree_view_column_set_reorderable(column, FALSE);
    gtk_tree_view_column_set_spacing(column,2);
    cell = gtk_cell_renderer_text_new();
    g_object_set(G_OBJECT(cell), "editable", FALSE, NULL);
    gtk_tree_view_column_set_title(column, "Name");
    gtk_tree_view_column_set_clickable(column, TRUE);

    gtk_tree_view_column_pack_start(column, cell, FALSE);
    gtk_tree_view_column_set_attributes(column, cell, 
		    "text", NAME_COLUMN, 
		    NULL);
     gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
    
      /* iconfile */
     column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_resizable(column, FALSE);
    gtk_tree_view_column_set_reorderable(column, FALSE);
    gtk_tree_view_column_set_spacing(column,2);
    cell = gtk_cell_renderer_text_new();
    g_object_set(G_OBJECT(cell), "editable", TRUE, NULL);
    g_signal_connect (G_OBJECT (cell), "edited",
		      G_CALLBACK (icon_changed), NULL);
    g_object_set_data (G_OBJECT (cell), "column", (gint *)ICON_COLUMN);

    
    gtk_tree_view_column_set_title(column, "Icon");
    gtk_tree_view_column_set_clickable(column, TRUE);

    gtk_tree_view_column_pack_start(column, cell, FALSE);
    gtk_tree_view_column_set_attributes(column, cell, 
		    "text", ICON_COLUMN, 
		    NULL);
     gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

  gtk_widget_show (xffm_theme_maker);
  g_signal_connect ((gpointer) xffm_theme_maker, "destroy",
                    G_CALLBACK (on_quit_clicked),
                    NULL);


 // g_hash_table_foreach  (hash_table, func,(gpointer)store);


  gtk_main ();
  return 0;
}

