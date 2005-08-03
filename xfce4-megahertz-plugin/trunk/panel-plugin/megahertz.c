/*  $Id$
 *
 *  Copyright © 2005 Wit Wilinski <madman@linux.bydg.org>
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

#include <libxfce4util/libxfce4util.h>

#include <panel/global.h>
#include <panel/controls.h>
#include <panel/icons.h>
#include <panel/plugins.h>

#define BORDER  8


typedef struct
{
    GtkWidget *eventbox;
    
    GtkWidget *table;

    GtkWidget *mhzlabel;
    GtkWidget *strlabel;

    GtkTooltips *tips;

    int size;

    int timeout_id;

    int ghz;
    int mhz;

}
Megahertz;

/* get speed from /sys */
static gboolean
megahertz_timeout (Megahertz *ac)
{
    char *small, *large;

    switch (ac->size)
    {
        case 0:
            small = "x-small";
            large = "large";
            break;
        case 1:
            small = "small";
            large = "x-large";
            break;
        case 2:
            small = "small";
            large = "x-large";
            break;
        default:
            small = "medium";
            large = "xx-large";
    }

	int mhz=0;
	int ghz=0;
	
	
	FILE *f;
	f=fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq", "r");
	if (f) {
	    fscanf(f, "%d", &mhz);
    fclose(f);
	}
	

    if (mhz/1000 > 1000) 
		ghz=1; // wyswietla GHz u gory
    else
		ghz=0; // wyswietla MHz u gory


    if (ghz != ac->ghz)
    {
		ac->ghz=ghz;
        char *markup = NULL;
        
			markup = g_strdup_printf ("<span size=\"%s\">%s</span>", 
                                  small, ghz ? "GHz" : "MHz");
        
        gtk_label_set_markup (GTK_LABEL (ac->strlabel), markup);

        g_free (markup);
    }
    
    if (mhz != ac->mhz)
    {
        char *markup = NULL;
        
        ac->mhz = mhz;
		if (ghz)
	
		
                markup = g_strdup_printf ("<span size=\"%s\">%.2f</span>", 
                                  large, mhz/1e6);
	
        else
                markup = g_strdup_printf ("<span size=\"%s\">%d</span>", 
                                  large, mhz/1000);
	

        gtk_label_set_markup (GTK_LABEL (ac->mhzlabel), markup);

        g_free (markup);
    }
    return TRUE;
}

static Megahertz *
megahertz_new (void)
{
    Megahertz *ac = g_new (Megahertz, 1);
    GtkWidget *align;
    
    ac->mhz=ac->ghz=-1;

    ac->size = settings.size;

    ac->tips = gtk_tooltips_new ();
    g_object_ref (ac->tips);
    gtk_object_sink (GTK_OBJECT (ac->tips));

    ac->eventbox = gtk_event_box_new ();
    gtk_widget_set_name (ac->eventbox, "xfce_panel");
    gtk_widget_show (ac->eventbox);

    align = gtk_alignment_new (0.5, 0.5, 0, 0);
    gtk_container_set_border_width (GTK_CONTAINER (align), 2);
    gtk_widget_show (align);
    gtk_container_add (GTK_CONTAINER (ac->eventbox), align);

    ac->table = gtk_table_new (2, 2, FALSE);
    gtk_table_set_col_spacings (GTK_TABLE (ac->table), 2);
    gtk_table_set_row_spacings (GTK_TABLE (ac->table), 0);
    gtk_widget_show (ac->table);
    gtk_container_add (GTK_CONTAINER (align), ac->table);

    ac->mhzlabel = gtk_label_new (NULL);
    gtk_widget_show (ac->mhzlabel);
    gtk_label_set_use_markup (GTK_LABEL (ac->mhzlabel), TRUE);
    gtk_table_attach (GTK_TABLE (ac->table), ac->mhzlabel,
                      0, 1, 0, 2, 0, 0, 0, 0);
    
    ac->strlabel = gtk_label_new (NULL);
    gtk_widget_show (ac->strlabel);
    gtk_label_set_use_markup (GTK_LABEL (ac->strlabel), TRUE);
    gtk_table_attach (GTK_TABLE (ac->table), ac->strlabel,
                      1, 2, 0, 1, 0, 0, 0, 0);
    
    megahertz_timeout (ac);

    ac->timeout_id =
	g_timeout_add (1000, (GSourceFunc) megahertz_timeout, ac);

    return ac;
}

static void
megahertz_free (Control * control)
{
    Megahertz *ac = control->data;

    g_return_if_fail (ac != NULL);

    if (ac->timeout_id)
    {
	g_source_remove (ac->timeout_id);
        ac->timeout_id = 0;
    }

    g_object_unref (ac->tips);

    g_free (ac);
}

static void
megahertz_attach_callback (Control * control, const char *signal,
		       GCallback callback, gpointer data)
{
    Megahertz *ac = control->data;

    g_signal_connect (ac->eventbox, signal, callback, data);
}

/* panel preferences */
static void
megahertz_set_size (Control * control, int size)
{
    Megahertz *ac = control->data;

    ac->size = size;

    ac->mhz = ac->ghz = -1;

    megahertz_timeout (ac);
}


/*  Clock panel control
 *  -------------------
*/
static gboolean
create_megahertz_control (Control * control)
{
    Megahertz *ac = megahertz_new ();

    gtk_container_add (GTK_CONTAINER (control->base), ac->eventbox);

    control->data = (gpointer) ac;
    control->with_popup = FALSE;

    gtk_widget_set_size_request (control->base, -1, -1);

    return TRUE;
}

G_MODULE_EXPORT void
xfce_control_class_init (ControlClass * cc)
{
    xfce_textdomain (GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");

    cc->name = "megahertz";
    cc->caption = _("Megahertz");

    cc->create_control = (CreateControlFunc) create_megahertz_control;

    cc->free = megahertz_free;
    cc->attach_callback = megahertz_attach_callback;

    cc->set_size = megahertz_set_size;

//    cc->create_options = megahertz_create_options;

    control_class_set_unique (cc, TRUE);
}

/* macro defined in plugins.h */
XFCE_PLUGIN_CHECK_INIT
