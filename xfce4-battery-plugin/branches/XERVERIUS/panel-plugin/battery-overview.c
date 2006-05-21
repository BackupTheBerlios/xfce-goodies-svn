/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright (c) 2006 Nick Schermer <nick@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#define BORDER 5

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <gtk/gtk.h>

#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4panel/xfce-panel-plugin.h>

#include "battery.h"

static gchar *
battery_get_status (BatteryStatus *bat)
{
    gchar *status;

    if (G_UNLIKELY (!bat->present))
        status = g_strdup (_("Battery Not Present"));
    
    else if (G_UNLIKELY (((bat->percentage == 100) && bat->charging)))
        status = g_strdup (_("Battery Fully Charged"));
    
    else if (bat->charging)
        status = g_strdup (_("Battery Charging"));
    
    else
        status = g_strdup (_("Battery Discharging"));

    return status;
}

static void
battery_add_overview (GtkWidget     *box,
                      BatteryStatus *bat)
{
    GtkWidget *hbox, *vbox, *label, *icon, *ibox, *expander;
    gchar *name, *status, *percentage, *time;

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), BORDER);
    
    /* Battery Icon */
    name = battery_icon_name (bat);

    icon = gtk_image_new_from_icon_name (name, GTK_ICON_SIZE_DIALOG);
    gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, TRUE, 0);
    gtk_misc_set_alignment (GTK_MISC (icon), 0.5, 0);
    gtk_misc_set_padding (GTK_MISC (icon), 10, 10);

    g_free (name);

    vbox = gtk_vbox_new (FALSE, 2);
    gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

    /* Status box */
    ibox = gtk_hbox_new (FALSE, BORDER*2);
    gtk_box_pack_start (GTK_BOX (vbox), ibox, TRUE, TRUE, 0);

    label = gtk_label_new (_("<b>Status:</b>"));
    gtk_box_pack_start (GTK_BOX (ibox), label, FALSE, FALSE, 0);
    gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

    status = battery_get_status (bat);

    label = gtk_label_new (status);
    gtk_box_pack_start (GTK_BOX (ibox), label, FALSE, FALSE, 0);
    
    g_free (status);
    
    /* Percentage */
    ibox = gtk_hbox_new (FALSE, BORDER*2);
    gtk_box_pack_start (GTK_BOX (vbox), ibox, TRUE, TRUE, 0);

    label = gtk_label_new (_("<b>Percentage:</b>"));
    gtk_box_pack_start (GTK_BOX (ibox), label, FALSE, FALSE, 0);
    gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    
    percentage = g_strdup_printf ("%d%%", bat->percentage);
    
    label = gtk_label_new (percentage);
    gtk_box_pack_start (GTK_BOX (ibox), label, FALSE, FALSE, 0);
    
    g_free (percentage);
    
    /* Time remaining */
    if (bat->time > 3600)
        time = g_strdup_printf (_("%d hr %d min"), bat->time / 3600, bat->time / 60 % 60);
    else if (bat->time > 0)
        time = g_strdup_printf (_("%d min"), bat->time / 60);
    else
        time = NULL;
    
    if (time)
    {    
        ibox = gtk_hbox_new (FALSE, BORDER*2);
        gtk_box_pack_start (GTK_BOX (vbox), ibox, TRUE, TRUE, 0);

        label = gtk_label_new (_("<b>Remaining Time:</b>"));
        gtk_box_pack_start (GTK_BOX (ibox), label, FALSE, FALSE, 0);
        gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
        gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    
        label = gtk_label_new (time);
        gtk_box_pack_start (GTK_BOX (ibox), label, FALSE, FALSE, 0);
    
        g_free (time);
    }
    
    expander = gtk_expander_new (NULL);
    gtk_box_pack_start (GTK_BOX (vbox), expander, TRUE, TRUE, 0);
    
    label = gtk_label_new (_("More..."));
    gtk_expander_set_label_widget (GTK_EXPANDER (expander), label);
}

static void
battery_overview_response (GtkWidget     *dialog, 
                           gint           response,
                           BatteryPlugin *battery)
{
    g_object_set_data (G_OBJECT (battery->plugin), "overview", NULL);

    gtk_widget_destroy (dialog);
}

gboolean
battery_overview (GtkWidget      *widget,
                  GdkEventButton *ev, 
                  BatteryPlugin  *battery)
{
    GtkWidget     *dialog, *window, *dialog_vbox;
    guint          i;
    BatteryStatus *bat;

#ifndef USE_NEW_DIALOG
    GtkWidget *header;
#endif

    if (ev->button != 1)
	return FALSE;
    
    window = g_object_get_data (G_OBJECT (battery->plugin), "overview");
    
    if (window)
        gtk_widget_destroy (window);

    DBG ("Show Overview");
    
#ifdef USE_NEW_DIALOG
    dialog = xfce_titled_dialog_new_with_buttons (_("Battery Information"),
                                                  NULL,
                                                  GTK_DIALOG_NO_SEPARATOR,
                                                  GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                                                  NULL);
    xfce_titled_dialog_set_subtitle (XFCE_TITLED_DIALOG (dialog),
                                     _("An overview of all the batteries in the system"));
#else
    dialog = gtk_dialog_new_with_buttons (_("Battery Information"), 
                                          GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (battery->plugin))),
                                          GTK_DIALOG_DESTROY_WITH_PARENT |
                                          GTK_DIALOG_NO_SEPARATOR,
                                          GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                                          NULL);
#endif
    
    gtk_window_set_position   (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
    gtk_window_set_icon_name  (GTK_WINDOW (dialog), "battery");
    
    g_object_set_data (G_OBJECT (battery->plugin), "overview", dialog);
    
    dialog_vbox = GTK_DIALOG (dialog)->vbox;

#ifndef USE_NEW_DIALOG
    header = xfce_create_header (NULL, _("Battery Information"));
    gtk_widget_set_size_request (GTK_BIN (header)->child, -1, 32);
    gtk_container_set_border_width (GTK_CONTAINER (header), BORDER);
    gtk_box_pack_start (GTK_BOX (dialog_vbox), header, FALSE, TRUE, 0);
#endif

    for (i = 0; i < battery->batteries->len; ++i)
    {
        bat = g_ptr_array_index (battery->batteries, i);
    
        battery_add_overview (dialog_vbox, bat);
    }

    g_signal_connect(dialog, "response",
        G_CALLBACK(battery_overview_response), battery);
    
    gtk_widget_show_all (dialog);

    return TRUE;
}
