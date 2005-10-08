/* mount-plugin.c */

/*
Copyright (C) 2005 Jean-Baptiste jb_dul@yahoo.com

This program is free software; you can redistribute it and/or 
modify it under the terms of the GNU General Public License 
as published by the Free Software Foundation; either 
version 2 of the License, or (at your option) any later 
version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "devices.h"
#include <gtk/gtk.h>

#include <stdlib.h>


/* for debugging, in 4.4 no more needed */
/* #include <libxfce4util/debug.h> */

/* for internationalization, by F. Nowak */
#include <libxfce4util/libxfce4util.h>

#include <libxfcegui4/libxfcegui4.h>

/* for panel 4.4 */
#include <libxfce4panel/xfce-panel-plugin.h>

/* no more needed in 4.4 panel */
/* #include <panel/xfce.h>
#include <panel/plugins.h> */

static GtkTooltips *tooltips = NULL;

#include "icons.h"

#define APP_NAME N_("Mount Plugin")

#define BORDER 6

/*--------- graphical interface ----------*/
typedef struct 
{
   XfcePanelPlugin *plugin;
	char * on_mount_cmd ;
	GtkWidget * button ;
	GtkWidget * menu ;
	GPtrArray * pdisks ; /* contains pointers to struct t_disk */
} t_mounter ;
/*---------------------------------*/

typedef struct
{
	GtkWidget * menu_item ;
	GtkWidget * hbox ;
	GtkWidget * label_disk ;
	GtkWidget * label_mount_info ;
	GtkWidget * progress_bar ;
} t_disk_display ;
/*------------------------------------------------*/

/*------------- settings dialog --------------------------*/
typedef struct
{
	t_mounter * mt ;
	GtkWidget * dialog ;
	/* options */
	GtkWidget * string_cmd ;	
}
t_mounter_dialog ;
/*------------------------------------------------------*/


/*---------------- on_activate_disk_display ---------------*/
static void 
on_activate_disk_display (GtkWidget * widget,t_disk * disk)
{
	t_mounter * mt ;
	if (disk != NULL)
	{
		if (disk->mount_info != NULL)
		{/*disk is mounted*/
			disk_umount(disk);
			/* disk->mount_info is freed by disk_mount */
		}
		else
		{/*disk is not mounted*/
			mt = (t_mounter*)g_object_get_data(G_OBJECT(widget),"mounter");
			disk_mount(disk,mt->on_mount_cmd);
			/* needs a refresh, a global refresh is done whe the window becomes visible*/
		}
	}
}

static void
mounter_set_size (XfcePanelPlugin *plugin, int size, t_mounter *mt)
{
   /* schrink the gtk button's image to new size - */
   gtk_widget_set_size_request (GTK_WIDGET(mt->button), size - 4, size - 4);
 
}

/*---------------------------------------------------------*/

/*-------------------- disk_display_new -----------------*/
/*create a new t_disk_display from t_disk infos */
static t_disk_display* 
disk_display_new (t_disk * disk, t_mounter * mounter)
{
	if (disk != NULL) 
	{
		t_disk_display * dd ;
		dd = g_new0 (t_disk_display, 1) ;
		dd->menu_item = gtk_menu_item_new();
		g_signal_connect (G_OBJECT(dd->menu_item), "activate",
		                  G_CALLBACK(on_activate_disk_display), disk);
		g_object_set_data (G_OBJECT(dd->menu_item), "mounter", (gpointer)mounter);
		
		dd->hbox = gtk_hbox_new (FALSE, 10);
		gtk_container_add (GTK_CONTAINER(dd->menu_item), dd->hbox);
		
		dd->label_disk = gtk_label_new (g_strconcat(disk->device, " -> ",
		                                disk->mount_point, NULL));
		/*change to uniform label size*/
		gtk_label_set_width_chars(GTK_LABEL(dd->label_disk),28);
		gtk_label_set_justify(GTK_LABEL(dd->label_disk),GTK_JUSTIFY_LEFT);
		gtk_box_pack_start(GTK_BOX(dd->hbox),dd->label_disk,FALSE,TRUE,0);
		
		dd->label_mount_info = gtk_label_new("");
		/*change to uniform label size*/
		gtk_label_set_width_chars(GTK_LABEL(dd->label_mount_info),25);
		gtk_label_set_use_markup(GTK_LABEL(dd->label_mount_info),TRUE);
		gtk_label_set_justify(GTK_LABEL(dd->label_mount_info),GTK_JUSTIFY_RIGHT);
		gtk_box_pack_start(GTK_BOX(dd->hbox),dd->label_mount_info,TRUE,TRUE,0);
		
		dd->progress_bar = gtk_progress_bar_new();
		gtk_box_pack_start(GTK_BOX(dd->hbox),dd->progress_bar,TRUE,TRUE,0);
		return dd ;
	}
	return NULL ;
}
/*-----------------------------------------------------------*/


/*-------------------- disk_display_refresh -----------------*/
static void 
disk_display_refresh(t_disk_display * disk_display, 
                                 t_mount_info * mount_info)
{
	TRACE("enters disk_display_refresh");
	if (disk_display != NULL)
	{
		if (mount_info != NULL)
		{	/* device is mounted */
			char * text ;
			char * used = get_size_human_readable(mount_info->used);
			//DBG("used is now : %s",used);
			char * size = get_size_human_readable(mount_info->size);
			//DBG("size is now : %s",size);
			char * avail = get_size_human_readable(mount_info->avail);
			//DBG("avail is now : %s",size);
			text = g_strdup_printf("[%s/%s] %s free", used ,size,avail );
			//DBG("text is now : %s",text);
			
			g_free(used);
			g_free(size);
			g_free(avail);
			//text = g_strdup_printf("mounted on : %s\t[%g Mb/%g Mb] %g Mb free",mount_info->mounted_on, mount_info->used, mount_info->size, mount_info->avail);
			gtk_label_set_text(GTK_LABEL(disk_display->label_mount_info),text);
			
			gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(disk_display->progress_bar),((gdouble)mount_info->percent / 100) );
			gtk_progress_bar_set_text(GTK_PROGRESS_BAR(disk_display->progress_bar),g_strdup_printf("%d",mount_info->percent));
			gtk_widget_show(GTK_WIDGET(disk_display->progress_bar));
		}
		else /* mount_info == NULL */
		{
			/*remove mount info */
			gtk_label_set_markup(GTK_LABEL(disk_display->label_mount_info),_("<span foreground=\"#FF0000\">not mounted</span>"));
			gtk_widget_hide(GTK_WIDGET(disk_display->progress_bar));

		}
	}
	TRACE("leaves disk_display_refresh");
}
/*----------------------------------------------*/

/*--------------- mounter_data_free -----------------*/
static void 
mounter_data_free (t_mounter * mt)
{
	TRACE ("enters mounter_data_free");
	
	disks_free (&(mt->pdisks));
	gtk_widget_destroy (GTK_WIDGET(mt->menu));
	mt->menu = NULL;
	
	TRACE ("leaves mounter_data_free");
}
/*----------------------------------------------*/

/*--------------- mounter_free -----------------*/
static void 
mounter_free (XfcePanelPlugin *plugin, t_mounter *mounter)
{
	TRACE ("enters mounter_free");
	
	mounter_data_free (mounter);
	
	g_free (mounter);
	
	TRACE ("leaves mounter_free");
}
/*----------------------------------------------*/
/*---------------- mounter_data_new --------------------------*/
static void 
mounter_data_new (t_mounter * mt)
{
	int i ;
	t_disk * disk ;
	t_disk_display * disk_display ;
	
	/*get static infos from /etc/fstab */
	mt->pdisks = disks_new();
	
	/* get dynamic infos on mounts */
	disks_refresh(mt->pdisks);
	
	/* menu with menu_item */
	mt->menu = gtk_menu_new();
	/* gtk_menu_shell_append(GTK_MENU_SHELL(mt->menu),gtk_menu_item_new_with_label("devices")); */
	/* gtk_menu_shell_append(GTK_MENU_SHELL(mt->menu),gtk_separator_menu_item_new());*/
	
	for(i=0;i < mt->pdisks->len;i++)
	{
		disk = g_ptr_array_index(mt->pdisks,i); //get the disk 
		disk_display = disk_display_new(disk,mt) ;// creates a disk_display
		disk_display_refresh(disk_display,disk->mount_info) ;//fill in mount infos
		gtk_menu_shell_append(GTK_MENU_SHELL(mt->menu),disk_display->menu_item);//add the menu_item to the menu
	}
	gtk_widget_show_all(mt->menu);
	
	return ;
}
/*------------------------------------------------------*/

/*---------------------- mounter_refresh ---------------*/
static void 
mounter_refresh (t_mounter * mt)
{
	TRACE ("enters mounter_refresh");
	
	mounter_data_free(mt);
	mounter_data_new(mt);

	TRACE ("leaves mounter_refresh");
	
}
/*---------------------------------------------------------*/

/* --------------plugin event --------------------------------*/
static gboolean 
on_button_press (GtkWidget *widget, GdkEventButton *event,
                                 t_mounter *mounter)
{
	TRACE ("enters on_button_pressed");
	if (mounter != NULL && event->button == 1)
	{
		
		mounter_refresh (mounter); // refreshs infos regarding mounts data
		gtk_menu_popup (GTK_MENU(mounter->menu),NULL,NULL,NULL,NULL,0,event->time);
		return TRUE;
	}
	TRACE ("leaves on_button_pressed");
	return FALSE ;
}
/*------------------------------------------------------*/

/*---------------------- mounter_read_config --------------------*/
static void
mounter_read_config (XfcePanelPlugin *plugin, t_mounter *mt)
{
	DBG ("enter read_config");
	
	 const char *value;
    char *file;
    XfceRc *rc;
    
    if ( !( file = xfce_panel_plugin_lookup_rc_file (plugin) ) )
        return;
    
    rc = xfce_rc_simple_open (file, TRUE);
    g_free (file);
    
    if (value = xfce_rc_read_entry(rc, "on_mount_cmd", NULL) /* && *value */ ) {
      mt->on_mount_cmd = g_strdup (value);
    }
    
    xfce_rc_close (rc);
    
    /* setup_mounter (mt); */

	 DBG ("leaves read_config");
}
/*-------------------------------------------------------*/

/*------------------- mounter_write_config -----------------------*/
static void
mounter_write_config (XfcePanelPlugin *plugin, t_mounter *mt)
{
	DBG("enter write_config");
	
	 XfceRc *rc;
    char *file;
    char value[20];

    if (!(file = xfce_panel_plugin_save_location (plugin, TRUE)))
        return;
    
    rc = xfce_rc_simple_open (file, FALSE);
    g_free (file);

    if (!rc)
        return;
	
	DBG ("save on_mount_cmd : %s", mt->on_mount_cmd);

   xfce_rc_write_entry (rc, "on_mount_cmd", 
         mt->on_mount_cmd ? mt->on_mount_cmd : "");
         
   xfce_rc_close (rc);

	DBG ("leaves write config");
}
/*--------------------------------------------*/

/*------------------- create_mounter -------------------*/
static t_mounter *
create_mounter_control (XfcePanelPlugin *plugin)
{
	TRACE ("enters create_mounter_control");

	t_mounter *mounter;
	
	mounter = g_new0(t_mounter,1);

	/* default mount command */
	mounter->on_mount_cmd = NULL;	
	
	mounter->plugin = plugin;
	
	if (!tooltips) 
    {
        tooltips = gtk_tooltips_new();
    }

	/*plugin button */
	
	GdkPixbuf * pb ;
	pb = gdk_pixbuf_new_from_inline (sizeof(icon_plugin), icon_plugin, FALSE, 
	                                 NULL);
	mounter->button = xfce_iconbutton_new_from_pixbuf (pb);
	gtk_button_set_relief (GTK_BUTTON(mounter->button), GTK_RELIEF_NONE);

	/* add_tooltip (GTK_WIDGET(mounter->button), _("devices")); */

   gtk_tooltips_set_tip (tooltips, GTK_WIDGET(mounter->button), _("devices"), 
                         NULL);

	/*-------------------------------------------------------------*/
	g_signal_connect (G_OBJECT(mounter->button), "button_press_event",
	                  G_CALLBACK(on_button_press), mounter);
	gtk_widget_show(mounter->button);
	
	/*get the data*/	
	mounter_data_new (mounter);
	
	TRACE ("leaves create_mounter_control");
	return mounter;
}


/*---------- free_mounter_dialog ---------------------*/
static void 
free_mounter_dialog(GtkWidget * widget, t_mounter_dialog * md)
{
	g_free(md);
}
/*----------------------------------------------------*/

/*---------------- mounter_apply_options ---------------*/
static void
mounter_apply_options (t_mounter_dialog *md) {
   const char * tmp;
	t_mounter * mt = md->mt;

	tmp = gtk_entry_get_text(GTK_ENTRY(md->string_cmd));
	
	g_free(mt->on_mount_cmd);
	if (tmp && *tmp)
		mt->on_mount_cmd = g_strdup(tmp);
	else
		mt->on_mount_cmd = NULL;
}

static void 
on_optionsDialog_response (GtkWidget *dlg, int response, t_mounter_dialog * md)
{
	mounter_apply_options (md);

   gtk_widget_destroy (md->dialog);
   xfce_panel_plugin_unblock_menu (md->mt->plugin);
   mounter_write_config (md->mt->plugin, md->mt);
}
/*------------------------------------------------------*/

/*-------------- entry_lost_focus -----------------------*/
/* This shows a way to update plugin settings when the user leaves a text entry,
by connecting to the "focus-out" event on the entry.*/

static gboolean
entry_lost_focus(t_mounter_dialog * md)
{
	mounter_apply_options (md);

	/* NB: needed to let entry handle focus-out as well */
	return FALSE;
}
/*----------------------------------------------------*/

/*----------------- mounter_create_options -------------*/
static void 
mounter_create_options (XfcePanelPlugin *plugin, t_mounter *mt)
{

   xfce_panel_plugin_block_menu (plugin);

   GtkWidget *dlg, *header;
   dlg = gtk_dialog_new_with_buttons (_("Edit Properties"), 
                GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
                GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
                GTK_STOCK_CLOSE, GTK_RESPONSE_OK, NULL);
                
   gtk_container_set_border_width (GTK_CONTAINER (dlg), 2);
   
   header = xfce_create_header (NULL, _("Mount devices"));
   gtk_widget_set_size_request (GTK_BIN (header)->child, -1, 32);
   gtk_container_set_border_width (GTK_CONTAINER (header), BORDER - 2);
   gtk_widget_show (header);
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), header, FALSE, TRUE, 0);

	GtkWidget *vbox, *label, *label2, *label3, *label4;
	GtkSizeGroup *sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	t_mounter_dialog * md;
	
	md = g_new0(t_mounter_dialog, 1);
	
	md->mt = mt; /* = (t_mounter *) control->data; */
	
	md->dialog = dlg; /* gtk_widget_get_toplevel (done); */
	
	/* don't set a border width, the dialog will take care of that */
	
	vbox = gtk_vbox_new (FALSE, BORDER);
	gtk_widget_show (vbox);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), GTK_WIDGET (vbox), TRUE, TRUE, 0);
	
	/* entries */
	
	label = gtk_label_new (_("command to execute after mounting a device"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_size_group_add_widget (sg, label);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
	
	label2 = gtk_label_new (_("(mount directory is added to the command)"));
	gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);
	gtk_size_group_add_widget (sg, label2);
	gtk_widget_show (label2);
	gtk_box_pack_start (GTK_BOX (vbox), label2, FALSE, FALSE, 0);
	
	label3 = gtk_label_new (_("eg. konqueror"));
	gtk_misc_set_alignment (GTK_MISC (label3), 0, 0.5);
	gtk_size_group_add_widget (sg, label3);
	gtk_widget_show (label3);
	gtk_box_pack_start (GTK_BOX (vbox), label3, FALSE, FALSE, 0);
	
	label4 = gtk_label_new (_("eg. xffm"));
	gtk_misc_set_alignment (GTK_MISC (label4), 0, 0.5);
	gtk_size_group_add_widget (sg, label4);
	gtk_widget_show (label4);
	gtk_box_pack_start (GTK_BOX (vbox), label4, FALSE, FALSE, 0);
	
	md->string_cmd = gtk_entry_new ();
	if (mt->on_mount_cmd != NULL)
		gtk_entry_set_text(GTK_ENTRY(md->string_cmd), g_strdup(mt->on_mount_cmd));
	gtk_widget_show(md->string_cmd);
	gtk_box_pack_start (GTK_BOX(vbox), md->string_cmd, FALSE, FALSE, 0);
	
	g_signal_connect_swapped (md->string_cmd, "focus-out-event",
			G_CALLBACK(entry_lost_focus), md);
				
	g_signal_connect (dlg, "response",
            G_CALLBACK(on_optionsDialog_response), md);
	
	gtk_widget_show (dlg);
}
/*----------------------------------------------------*/


/* extensions for panel 4.4 */

static void 
mount_construct (XfcePanelPlugin *plugin)
{   
    xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    t_mounter *mounter;

    mounter = create_mounter_control (plugin);

    mounter_read_config (plugin, mounter);
    
    g_signal_connect (plugin, "free-data", G_CALLBACK (mounter_free), mounter);
    
    g_signal_connect (plugin, "save", G_CALLBACK (mounter_write_config), mounter);
    
    xfce_panel_plugin_menu_show_configure (plugin);
    g_signal_connect (plugin, "configure-plugin", 
                      G_CALLBACK (mounter_create_options), mounter);
    
    g_signal_connect (plugin, "size-changed", G_CALLBACK (mounter_set_size), 
                         mounter);
    
    /* g_signal_connect (plugin, "orientation-changed", 
                      G_CALLBACK (monitor_set_orientation), mounter); */
    
    gtk_container_add (GTK_CONTAINER(plugin), mounter->button);

    xfce_panel_plugin_add_action_widget (plugin, mounter->button);
	
}

XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL (mount_construct);


/* end extensions for panel 4.4 */

