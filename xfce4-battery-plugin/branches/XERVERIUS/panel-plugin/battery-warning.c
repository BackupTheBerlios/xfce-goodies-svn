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
 
#define LOW_TIMEOUT      5 * 60 * 1000 /* 5 minutes */
#define CRITICAL_TIMEOUT 1 * 60 * 1000 /* 1 minute */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <gtk/gtk.h>

#include <libxfce4panel/xfce-panel-plugin.h>

#include "battery.h"
#include "battery-warning.h"

static gint
warning_timeout_id = 0;

static ActionType
warning_active_action = NONE;

void
battery_warning_stop (void)
{
    if (warning_timeout_id)
    {
        DBG ("Timout %d stopped", warning_timeout_id);
        g_source_remove (warning_timeout_id);
        warning_timeout_id = 0;
    }
}

static void
battery_warning_response (GtkWidget     *dialog, 
                          gint           response,
                          BatteryPlugin *battery)
{
    g_object_set_data (G_OBJECT (battery->plugin), "warning", NULL);

    /* "Don't show again" button clicked */
    if (response == GTK_RESPONSE_OK)
        battery_warning_stop ();

    gtk_widget_destroy (dialog);
}

static void
battery_warning_dialog (BatteryPlugin *battery,
                        const gchar   *title,
                        const gchar   *message)
{
    GtkWidget *dialog;
    GtkWidget *window = g_object_get_data (G_OBJECT (battery->plugin), "warning");

    if (G_UNLIKELY (window))
    {
        g_object_set_data (G_OBJECT (battery->plugin), "warning", NULL);
        gtk_widget_destroy (window);
    }
    
    dialog = gtk_message_dialog_new_with_markup (NULL,
                                                 GTK_DIALOG_MODAL,
                                                 GTK_MESSAGE_WARNING,
                                                 GTK_BUTTONS_NONE,
                                                 title);
    
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                              message);

    g_object_set_data (G_OBJECT (battery->plugin), "warning", dialog);

    gtk_window_set_icon_name (GTK_WINDOW (dialog), "gtk-dialog-warning");
    gtk_window_set_keep_above(GTK_WINDOW (dialog), TRUE);
    gtk_window_stick (GTK_WINDOW (dialog));

    gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
    
    gtk_dialog_add_button (GTK_DIALOG (dialog), _("Remind me later"),  GTK_RESPONSE_DELETE_EVENT);
    gtk_dialog_add_button (GTK_DIALOG (dialog), _("Don't show again"), GTK_RESPONSE_OK);
    
    g_signal_connect(dialog, "response",
        G_CALLBACK(battery_warning_response), battery);
        
    gtk_widget_show (dialog);
}

static void
battery_display_warning (BatteryPlugin *battery,
                         ActionType     type)
{
    switch (type)
    {
        case CHARGED:
            battery_warning_dialog (battery,
                                    _("<b>Your battery is now fully charged.</b>"),
                                    _("You should consider plugging out your AC cable to not overcharge your battery."));
            break;
        
        case LOW:
            battery_warning_dialog (battery,
                                    _("<b>Your battery is running low.</b>"),
                                    _("You should consider plugging in or shutting down your computer soon to avoid possible data loss."));
            break;
    
        case CRITICAL:
            battery_warning_dialog (battery,
                                    _("<b>Your battery has reached critical status.</b>"),
                                    _("You should plug in or shutdown your computer now to avoid possible data loss."));
            break;
    
        default:
            break;
    }
}

static gboolean
battery_active_warning (gpointer data)
{
    BatteryPlugin *battery = (BatteryPlugin *) data;

    DBG ("Display battery warning again");

    if (warning_active_action != NONE)
    {
        battery_display_warning (battery, warning_active_action);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
    
void
battery_warning (BatteryPlugin *battery,
                 ActionType     type)
{
    if (G_LIKELY (type != warning_active_action))
        warning_active_action = type;
    
    battery_warning_stop ();
        
    /* Start a new timout, if needed */
    if (type == LOW)
        warning_timeout_id = 
            g_timeout_add (LOW_TIMEOUT, battery_active_warning, battery);
    else if (type == CRITICAL)
        warning_timeout_id = 
            g_timeout_add (CRITICAL_TIMEOUT, battery_active_warning, battery);

    DBG ("New timeout started: %d", warning_timeout_id);
    
    battery_display_warning (battery, type);
}
