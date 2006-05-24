/* vim: set expandtab ts=8 sw=4: */

/*  Copyright (c) 2006 Nick Schermer <nick@xfce.org>
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

#define SPACING           2
#define BORDER            4
#define PROGRESSBAR_WIDTH 8
#define SMALL_PANEL_SIZE  34

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <gtk/gtk.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4panel/xfce-panel-plugin.h>

#include "battery.h"
#include "battery-hal.h"
#include "battery-dialogs.h"
#include "battery-warning.h"
#include "battery-overview.h"

static void
battery_open (BatteryPlugin *battery);

static void
battery_construct (XfcePanelPlugin *plugin);

/* Register Plugin */
XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL (battery_construct);

/* Battery Panel Widgets */
static void
battery_tooltip (BatteryPlugin *battery,
                 GString       *tooltip,
                 BatteryStatus *bat)
{
    if (bat->charging && bat->present) /* Charging */
    {
        if (bat->percentage == 100)
            tooltip = g_string_append (tooltip, _("Fully Charged (100%)"));
        
        else if (battery->tip_time && G_LIKELY (bat->time > 3600))
            g_string_append_printf (tooltip, _("Charging (%d%% completed)\n%d hr %d min remaining"), bat->percentage, bat->time / 3600, bat->time / 60 % 60);
        
        else if (battery->tip_time && G_LIKELY (bat->time > 0))
            g_string_append_printf (tooltip, _("Charging (%d%% completed)\n%d min remaining"), bat->percentage, bat->time / 60);
        
        else
            g_string_append_printf (tooltip, _("Charging (%d%% completed)"), bat->percentage);
    }
    else if (bat->present)/* Discharging */
    {
        if (battery->tip_time && bat->time > 3600)
            g_string_append_printf (tooltip, _("%d hr %d min (%d%%) remaining"), bat->time / 3600, bat->time / 60 % 60, bat->percentage);
        
        else if (battery->tip_time && bat->time > 0)
            g_string_append_printf (tooltip, _("%d min (%d%%) remaining"), bat->time / 60, bat->percentage);
        
        else
            g_string_append_printf (tooltip, _("%d%% remaining"), bat->percentage);
    }
    else /* Battery not present in system */
    {
        tooltip = g_string_append (tooltip, _("Battery not present"));
    }    
}

static const gchar *
battery_icon_group (guint percentage)
{
    if (percentage      <= 10)
        return "000";
    
    else if (percentage <= 30)
        return "020";
    
    else if (percentage <= 50)
        return "040";
    
    else if (percentage <= 70)
        return "060";
    
    else if (percentage <= 90)
        return "080";
    
    else /* (percentage <= 100) */
        return "100";
}

gchar *
battery_icon_name (BatteryStatus *bat)
{
    gchar *name;

    if (G_UNLIKELY (!bat->present))
        name = g_strdup ("battery-missing");
    
    else if (G_UNLIKELY (((bat->percentage == 100) && bat->charging)))
        name = g_strdup ("battery-charged");
    
    else if (bat->charging)
        name = g_strconcat ("battery-charging-",
                            battery_icon_group (bat->percentage),
                            NULL);
    
    else
        name = g_strconcat ("battery-discharging-",
                            battery_icon_group (bat->percentage),
                            NULL);
    
    return name;
}

static void
battery_widget_icon (BatteryPlugin *battery,
                     BatteryStatus *bat)
{
    gchar     *name;
    guint      psize;
    GdkPixbuf *pixbuf = NULL;

    if (G_UNLIKELY (!battery->show_icon ||
                    !GTK_IS_WIDGET (battery->icon)))
        return;
    
    name = battery_icon_name (bat);    
    
    /* Only update icon if it's different from new one */
    if (G_UNLIKELY (strcmp (battery->iconname, name) != 0))
    {
        DBG ("Set (%s) to icon: %s", battery->iconname, name);
    
        /* Get plugin size - the table spacing */
        psize = xfce_panel_plugin_get_size (battery->plugin) - 2 * BORDER;
    
        pixbuf = xfce_themed_icon_load (name, psize);
    
        gtk_image_set_from_pixbuf (GTK_IMAGE (battery->icon), 
                                   pixbuf);
    
        g_object_unref (pixbuf);
    
        /* Set new icon name */
        g_free (battery->iconname);
        battery->iconname = g_strdup (name);
    }
    
    g_free (name);
}

static void
battery_widget_label (BatteryPlugin *battery,
                      BatteryStatus *bat)
{
    gchar    *label;
    gchar    *strtime;
    gchar    *strperc;
    gboolean  small;

    if (!(battery->show_percentage | battery->show_time))
        return;
    
    if (G_LIKELY (!GTK_IS_WIDGET (battery->label)))
        return;
    
    small = xfce_panel_plugin_get_size (battery->plugin) <= SMALL_PANEL_SIZE;
    
    strperc = g_strdup_printf ("%d%%", bat->percentage);
    
    if (G_LIKELY (bat->time > 0))
        strtime = g_strdup_printf ("%d:%02d", bat->time / 3600, bat->time / 60 % 60);
    else
        strtime = g_strdup ("0:00");

    label = g_strconcat (small ? "<small>" : "",
                         battery->show_percentage ? strperc : "",
                         battery->show_percentage && battery->show_time ? "\n" : "",
                         battery->show_time ? strtime : "",
                         small ? "</small>" : "",
                         NULL);
    
    /* Set the label, no checking here because it's almost different any time */
    gtk_label_set_label (GTK_LABEL(battery->label), label);
    
    g_free (strtime);
    g_free (strperc);
    g_free (label);
}

static void
battery_widget_progressbar (BatteryPlugin *battery,
                            BatteryStatus *bat)
{
    if (G_UNLIKELY (!battery->show_progressbar ||
                    !GTK_IS_WIDGET (battery->progressbar)))
        return;
    
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (battery->progressbar), 
                                   bat->percentage / 100.0);
}

void
battery_widgets (BatteryPlugin *battery)
{
    GtkOrientation  orientation;
    GtkWidget      *box;

    g_return_if_fail (battery->running);

    orientation = xfce_panel_plugin_get_orientation (battery->plugin);
    
    /* Destroy all widgets */
    if (battery->ebox)
        gtk_widget_destroy (battery->ebox);

    /* Rebuild the plugin */
    battery->ebox = gtk_event_box_new ();

    if (orientation == GTK_ORIENTATION_HORIZONTAL)
        box = gtk_hbox_new (FALSE, SPACING);
    else 
        box = gtk_vbox_new (FALSE, SPACING);

    gtk_container_set_border_width (GTK_CONTAINER (box), BORDER);
    gtk_container_add (GTK_CONTAINER (battery->ebox), box);

    if (battery->show_icon)
    {
        battery->icon = gtk_image_new_from_icon_name ("battery-missing", GTK_ICON_SIZE_BUTTON);
        gtk_box_pack_start (GTK_BOX (box), battery->icon, FALSE, FALSE, 0);
        battery->iconname = g_strdup ("");
    }

    if (battery->show_progressbar)
    {
        battery->progressbar = gtk_progress_bar_new ();
        gtk_box_pack_start (GTK_BOX (box), battery->progressbar, FALSE, FALSE, 0);

        if (orientation == GTK_ORIENTATION_HORIZONTAL)
        {
            gtk_widget_set_size_request (battery->progressbar, PROGRESSBAR_WIDTH, -1);
            gtk_progress_bar_set_orientation (GTK_PROGRESS_BAR (battery->progressbar), GTK_PROGRESS_BOTTOM_TO_TOP);
        }
        else
        {
            gtk_widget_set_size_request (battery->progressbar, -1, PROGRESSBAR_WIDTH);
            gtk_progress_bar_set_orientation (GTK_PROGRESS_BAR (battery->progressbar), GTK_PROGRESS_LEFT_TO_RIGHT);
        }
    }
    
    if (battery->show_percentage || battery->show_time)
    {
        battery->label = gtk_label_new ("");
        gtk_box_pack_start (GTK_BOX (box), battery->label, FALSE, FALSE, 0);
        gtk_label_set_use_markup (GTK_LABEL (battery->label), TRUE);
    }
    
    /* Display Widgets and (re)connect signals */
    gtk_widget_show_all (battery->ebox);

    g_signal_connect (battery->ebox, "button_press_event",
        G_CALLBACK (battery_overview), battery);

    gtk_container_add (GTK_CONTAINER (battery->plugin), battery->ebox);
    xfce_panel_plugin_add_action_widget (battery->plugin, battery->ebox);
}

void
battery_error_widget (BatteryPlugin *battery)
{
    guint      psize;
    GdkPixbuf *pixbuf = NULL;

    DBG ("Display the error widget");

    battery->ebox = gtk_event_box_new ();

    /* Create icon */
    psize = xfce_panel_plugin_get_size (battery->plugin) - SPACING*2;
    pixbuf = xfce_themed_icon_load("battery-missing", psize);
    battery->icon = gtk_image_new_from_pixbuf (pixbuf);
    g_object_unref (pixbuf);

    gtk_container_add (GTK_CONTAINER (battery->ebox), battery->icon);
    gtk_widget_show_all (battery->ebox);

    gtk_container_add (GTK_CONTAINER (battery->plugin), battery->ebox);
    xfce_panel_plugin_add_action_widget (battery->plugin, battery->ebox);
}

/* Battery Actions */

static void
battery_run_action (ActionType     type,
                    BatteryPlugin *battery,
                    Action         action,
                    gchar         *command)
{
    switch (action)
    {
        case MESSAGE:
            battery_warning (battery, type);
            break;
        
        case COMMAND:
            if (!xfce_exec (command, FALSE, FALSE, NULL))
                goto failed;
            break;
        
        case TERMINAL:
            if (!xfce_exec (command, TRUE, FALSE, NULL))
                goto failed;
            break;
        
        default:
            break;
    }
    
    return;
    
    failed:
        g_warning (_("The battery monitor was unable to execute following command: %s"), command);
}

/* Update the plugin */
void
battery_update_plugin (BatteryPlugin *battery)
{
    guint          i;
    GString       *tooltip;
    BatteryStatus *bat;
    
    g_return_if_fail (battery->running);

    tooltip = g_string_new ("");

    /* Loop for each battery that was found */
    for (i = 0; i < battery->batteries->len; ++i)
    {
        bat = g_ptr_array_index (battery->batteries, i);
    
        /* If this battery is the battery that is shown in the panel, update it */
        if (G_LIKELY (i == battery->show_battery))
        {
            battery_widget_progressbar (battery, bat);
            battery_widget_label       (battery, bat);
            battery_widget_icon        (battery, bat);
        }
        
        /* Build the tooltip */
        if (G_LIKELY (battery->batteries->len == 1))
        {
            battery_tooltip (battery, tooltip, bat);
        }
        else
        {
            g_string_append_printf (tooltip, _("Battery %d:\n"), i+1);
        
            battery_tooltip (battery, tooltip, bat);
            
            if (G_LIKELY (i < (battery->batteries->len-1)))
                tooltip = g_string_append (tooltip, "\n\n");
        }
        
        /* Check for actions. The battery_run_action is triggered only
           once to prevent useless behaviours */
        if (bat->charging && G_LIKELY (bat->present))
        {
            if (G_UNLIKELY (bat->percentage == 100))
            {
                if (bat->active_action != CHARGED)
                {
                    DBG ("Battery is fully charged -> run action");
                    bat->active_action = CHARGED;
                    
                    battery_run_action (CHARGED,
                                        battery,
                                        battery->action_charged,
                                        battery->command_charged);
                }
            }
            else
            {
                if (bat->active_action != NONE)
                {
                    /* Stop the warning timout */
                    battery_warning_stop ();
                    
                    /* Reset the current battery action */
                    bat->active_action = NONE;
                    
                    /* Check if the warning message is shown, if so, destroy it */
                    GtkWidget *warning = g_object_get_data (G_OBJECT (battery->plugin), "warning");
                    
                    if (G_UNLIKELY (warning))
                    {
                        DBG ("Destroy dialog");
                        g_object_set_data (G_OBJECT (battery->plugin), "warning", NULL);
                        gtk_widget_destroy (warning);
                    }
                }
            }
        }
        else if (G_LIKELY (bat->present)) /* Not charging, but present in system */
        {
            if (bat->percentage <= battery->perc_critical)
            {
                if (bat->active_action != CRITICAL)
                {
                    DBG ("Battery status is critical -> run action");
                    bat->active_action = CRITICAL;
                    
                    battery_run_action (CRITICAL,
                                        battery,
                                        battery->action_critical,
                                        battery->command_critical);
                }
            }
            else if (bat->percentage <= battery->perc_low)
            {
                if (bat->active_action != LOW)
                {
                    DBG ("Battery status is low -> run action");
                    bat->active_action = LOW;
                    
                    battery_run_action (LOW,
                                        battery,
                                        battery->action_low,
                                        battery->command_low);
                }
            }
            else
            {
                if (bat->active_action != NONE)
                {
                    /* Stop the warning timout */
                    battery_warning_stop ();
                    
                    /* Reset the current battery action */
                    bat->active_action = NONE;
                    
                    /* Check if the warning message is shown, if so, destroy it */
                    GtkWidget *warning = g_object_get_data (G_OBJECT (battery->plugin), "warning");
                    
                    if (G_UNLIKELY (warning))
                    {
                        DBG ("Destroy dialog");
                        g_object_set_data (G_OBJECT (battery->plugin), "warning", NULL);
                        gtk_widget_destroy (warning);
                    }
                }
            }
        }
    }
    
    gtk_tooltips_set_tip (battery->tooltip,
                          battery->ebox,
                          tooltip->str,
                          NULL);
    
    g_string_free (tooltip, TRUE);
}

static void
battery_open (BatteryPlugin *battery)
{
    XfceRc      *rc;
    gchar       *file;
    const gchar *s;

    file = xfce_panel_plugin_save_location (battery->plugin, FALSE);

    if (G_UNLIKELY (!file))
        return;
    
    DBG("Read from file: %s", file);
    
    rc = xfce_rc_simple_open (file, FALSE);
    g_free (file);
    
    xfce_rc_set_group (rc, "Properties");
    
    battery->show_battery     = xfce_rc_read_int_entry  (rc, "show_battery",     0);
    
    battery->show_icon        = xfce_rc_read_bool_entry (rc, "show_icon",        TRUE);
    battery->show_progressbar = xfce_rc_read_bool_entry (rc, "show_progressbar", TRUE);
    battery->show_percentage  = xfce_rc_read_bool_entry (rc, "show_percentage",  TRUE);
    battery->show_time        = xfce_rc_read_bool_entry (rc, "show_time",        TRUE);
    
    battery->tip_time         = xfce_rc_read_bool_entry (rc, "tip_time",         TRUE);
    
    battery->action_critical  = xfce_rc_read_int_entry  (rc, "action_critical",  1);
    battery->action_low       = xfce_rc_read_int_entry  (rc, "action_low",       1);
    battery->action_charged   = xfce_rc_read_int_entry  (rc, "action_charged",   0);
    
    battery->perc_critical    = xfce_rc_read_int_entry  (rc, "perc_critical",    10);
    battery->perc_low         = xfce_rc_read_int_entry  (rc, "perc_low",         20);
    
    if((s = xfce_rc_read_entry (rc, "command_critical", "")) != NULL);
        battery->command_critical = g_strdup (s);
    
    if ((s = xfce_rc_read_entry (rc, "command_low", NULL)) != NULL)
        battery->command_low = g_strdup (s);
    
    if ((s = xfce_rc_read_entry (rc, "command_charged", NULL)) != NULL)
        battery->command_charged = g_strdup (s);
    
    xfce_rc_close (rc);
}

void
battery_save (XfcePanelPlugin *plugin,
              BatteryPlugin   *battery)
{
    XfceRc *rc;
    gchar  *file;

    file = xfce_panel_plugin_save_location (plugin, TRUE);

    if (G_UNLIKELY (!file))
        return;
    
    DBG("Save to file: %s", file);
    
    rc = xfce_rc_simple_open (file, FALSE);
    g_free (file);
    
    xfce_rc_set_group (rc, "Properties");
    
    xfce_rc_write_int_entry  (rc, "show_battery",     battery->show_battery);
    
    xfce_rc_write_bool_entry (rc, "show_icon",        battery->show_icon);
    xfce_rc_write_bool_entry (rc, "show_progressbar", battery->show_progressbar);
    xfce_rc_write_bool_entry (rc, "show_percentage",  battery->show_percentage);
    xfce_rc_write_bool_entry (rc, "show_time",        battery->show_time);

    xfce_rc_write_bool_entry (rc, "tip_time",         battery->tip_time);
  
    xfce_rc_write_int_entry  (rc, "action_critical",  battery->action_critical);
    xfce_rc_write_int_entry  (rc, "action_low",       battery->action_low);
    xfce_rc_write_int_entry  (rc, "action_charged",   battery->action_charged);
    
    xfce_rc_write_int_entry  (rc, "perc_critical",    battery->perc_critical);
    xfce_rc_write_int_entry  (rc, "perc_low",         battery->perc_low);

    xfce_rc_write_entry (rc, "command_critical", battery->command_critical ? battery->command_critical : "");
    xfce_rc_write_entry (rc, "command_low",      battery->command_low      ? battery->command_low      : "");
    xfce_rc_write_entry (rc, "command_charged",  battery->command_charged  ? battery->command_charged  : "");

    xfce_rc_close (rc);
}

static void
battery_free (XfcePanelPlugin *plugin,
              BatteryPlugin   *battery)
{
    guint          i;
    BatteryStatus *bat;
    GtkWidget     *configure, *warning, *overview;

    /* Remove the batteries array */
    for (i = battery->batteries->len; i--;)
    {
        bat = g_ptr_array_index (battery->batteries, i);
        g_ptr_array_remove_fast (battery->batteries, bat);
        battery_remove (bat);
    }
    g_ptr_array_free (battery->batteries, TRUE);
    
    /* Remove tooltip */
    gtk_tooltips_set_tip (battery->tooltip, battery->ebox, NULL, NULL);
    g_object_unref (battery->tooltip);

    /* Stop HAL Monitor */
    battery_stop_monitor (battery);
    
    /* Stop battery warning timout */
    battery_warning_stop ();

    /* Destroy windows (properties, warnings, errors  */
    configure = g_object_get_data (G_OBJECT (plugin), "configure");
    warning   = g_object_get_data (G_OBJECT (plugin), "warning");
    overview  = g_object_get_data (G_OBJECT (plugin), "overview");
    
    if (configure)
        gtk_widget_destroy (configure);
    if (warning)
        gtk_widget_destroy (warning);
    if (overview)
        gtk_widget_destroy (overview);
    
    /* Destroy Panel Widgets */
    if (battery->icon)
        gtk_widget_destroy (battery->icon);
    if (battery->progressbar)
        gtk_widget_destroy (battery->progressbar);
    if (battery->label)
        gtk_widget_destroy (battery->label);
    if (battery->ebox)
        gtk_widget_destroy (battery->ebox);
    
    g_free (battery->iconname);
    
    g_free (battery->command_critical);
    g_free (battery->command_low);
    g_free (battery->command_charged);

    battery->plugin = NULL;
    
    g_free (battery);
    
    DBG ("Plugin Freed");
}

static gboolean
battery_set_size (XfcePanelPlugin *plugin, 
                  gint             wsize,
                  BatteryPlugin   *battery)
{
    DBG ("Set plugin size");

    /* Update size of the plugin */
    if (xfce_panel_plugin_get_orientation (plugin) == GTK_ORIENTATION_HORIZONTAL)
        gtk_widget_set_size_request (GTK_WIDGET (plugin), -1, wsize);
    else
        gtk_widget_set_size_request (GTK_WIDGET (plugin), wsize, -1);

    /* Reset icon name so it will be updated */
    g_free (battery->iconname);
    battery->iconname = g_strdup ("");

    battery_update_plugin (battery);
    
    return TRUE;
}

static void
battery_orientation_changed (XfcePanelPlugin *plugin, 
                             GtkOrientation   orientation,
                             BatteryPlugin   *battery)
{
    DBG ("Change orientation");
    
    battery_widgets (battery);
}

static void
battery_manual_update (GtkWidget     *mi,
                       BatteryPlugin *battery)
{
    if (G_LIKELY (battery->running))
    {
        battery_rescan_batteries (battery);
        battery_update_plugin (battery);
    }
}

static BatteryPlugin *
battery_plugin_new (XfcePanelPlugin *plugin)
{
    BatteryPlugin *battery;
    GtkWidget     *mi;

    battery = g_new0 (BatteryPlugin, 1);
    battery->plugin = plugin;

    battery->tooltip = gtk_tooltips_new();
    g_object_ref (battery->tooltip);

    battery->batteries = g_ptr_array_new ();

    battery_open (battery);

    if (G_UNLIKELY (battery_start_monitor (battery) == FALSE))
    {
        battery_error_widget (battery);
        return battery;
    }
    
    /* Check if the default battery exists */
    if (battery->show_battery + 1 > battery->batteries->len)
        battery->show_battery = 0;
    
    /* Tell everyone hal is running and batteries are all fine */
    battery->running = TRUE;
    
    /* Add refresh options to right click menu */
    mi = gtk_image_menu_item_new_from_stock ("gtk-refresh", NULL);
    gtk_widget_show (mi);
    g_signal_connect(mi, "activate", 
        G_CALLBACK(battery_manual_update), battery);
    xfce_panel_plugin_menu_insert_item (plugin, GTK_MENU_ITEM (mi));
    
    battery_widgets (battery);
    
    return battery;
}

static void 
battery_construct (XfcePanelPlugin *plugin)
{
    BatteryPlugin *battery;

    xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
    
    battery = battery_plugin_new (plugin);

    /* Connect Plugin Signals */
    g_signal_connect (plugin, "free-data", 
        G_CALLBACK (battery_free), battery);

    g_signal_connect (plugin, "save", 
        G_CALLBACK (battery_save), battery);

    g_signal_connect (plugin, "size-changed",
        G_CALLBACK (battery_set_size), battery);

    g_signal_connect (plugin, "orientation-changed", 
        G_CALLBACK (battery_orientation_changed), battery);
 
    xfce_panel_plugin_menu_show_configure (plugin);
    g_signal_connect (plugin, "configure-plugin", 
        G_CALLBACK (battery_configure), battery);
}
