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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>

#include <dbus/dbus-glib-lowlevel.h>
#include <hal/libhal.h>

#include <libxfce4panel/xfce-panel-plugin.h>

#include "battery.h"
#include "battery-hal.h"

static LibHalContext  *context;
static DBusConnection *dbus_connection;

static void
battery_store_propery (LibHalContext *ctx,
                       const char    *udi,
                       const char    *key,
                       BatteryStatus *bat,
                       DBusError      error)
{
    if (bat->percentage != libhal_device_get_property_int (ctx, udi, "battery.charge_level.percentage", &error))
    {
        bat->percentage = libhal_device_get_property_int (ctx, udi, "battery.charge_level.percentage", &error);
        return;
    }
  
    if (G_LIKELY (libhal_device_property_exists  (ctx, udi, "battery.remaining_time", &error)       &&
                  libhal_device_get_property_int (ctx, udi, "battery.remaining_time", &error) > 0 ) &&
                  libhal_device_get_property_int (ctx, udi, "battery.remaining_time", &error) != bat->time)
    {
        bat->time = libhal_device_get_property_int (ctx, udi, "battery.remaining_time", &error);
        return;
    }
    
    if (G_UNLIKELY (strcmp (key, "battery.rechargeable.is_discharging") == 0))
    {
        bat->charging = !libhal_device_get_property_bool (ctx, udi, "battery.rechargeable.is_discharging", &error);
        return;
    }
    
    if (G_UNLIKELY (strcmp (key, "battery.present") == 0))
    {
        bat->present  = libhal_device_get_property_bool (ctx, udi, "battery.present", &error);
        return;
    }
}

static void
hal_property_modified (LibHalContext *ctx,
                       const char    *udi,
                       const char    *key,
                       dbus_bool_t    is_removed,
                       dbus_bool_t    is_added)
{
    unsigned int   i;
    DBusError      error;
    BatteryPlugin *battery;
    BatteryStatus *bat;

    /* Only allow battery* keys */
    if (strncmp (key, "battery", 7) != 0)
        return;
    
    battery = libhal_ctx_get_user_data (ctx);
    
    g_return_if_fail (ctx == context);

    dbus_error_init (&error);
    
    for (i = battery->batteries->len; i--; )
    {
        bat = g_ptr_array_index (battery->batteries, i);
    
        if (G_LIKELY (strcmp (bat->udi, udi) == 0))
        {
            battery_store_propery (ctx, udi, key, bat, error);
            battery_update_plugin (battery);
            
            break;
        }
    }
    
    if (G_UNLIKELY (dbus_error_is_set (&error)))
    {
        DBG ("DBus Error: %s: %s", error.name, error.message);
        dbus_error_free (&error);
    }
}

void
battery_remove (BatteryStatus *bat)
{
    DBusError error;
    dbus_error_init (&error);

    if (dbus_error_is_set (&error))
    {
        DBG ("DBus Error: %s: %s", error.name, error.message);
        dbus_error_free (&error);
    }

    g_free (bat->udi);
    g_free (bat);
}

#ifdef DUMMIES
static void
battery_add_dummies (BatteryPlugin *battery)
{
    BatteryStatus *bat;

    DBG ("Interting dummy batteries");

    /* Dummy 1 */
    bat = g_new0 (BatteryStatus, 1);

    bat->udi              = g_strdup ("/org/freedesktop/Hal/devices/acpi_BAT97");
    bat->charging         = FALSE;
    bat->present          = TRUE;
    bat->percentage       = 81;
    bat->time             = 2000;
    bat->active_action    = NONE;

    g_ptr_array_add (battery->batteries, bat);

    /* Dummy 2 */
    bat = g_new0 (BatteryStatus, 1);

    bat->udi              = g_strdup ("/org/freedesktop/Hal/devices/acpi_BAT98");
    bat->charging         = TRUE;
    bat->present          = TRUE;
    bat->percentage       = 44;
    bat->time             = 3800;
    bat->active_action    = NONE;
    
    g_ptr_array_add (battery->batteries, bat);
    
    /* Dummy 3 */
    bat = g_new0 (BatteryStatus, 1);

    bat->udi              = g_strdup ("/org/freedesktop/Hal/devices/acpi_BAT99");
    bat->charging         = FALSE;
    bat->present          = FALSE;
    bat->percentage       = 0;
    bat->time             = 0;
    bat->active_action    = NONE;

    g_ptr_array_add (battery->batteries, bat);

}
#endif

static void
battery_refresh_settings (BatteryStatus *bat,
                          const gchar   *udi,
                          DBusError      error)
{
    bat->charging      = !libhal_device_get_property_bool(context, udi, "battery.rechargeable.is_discharging", &error);
    bat->present       = libhal_device_get_property_bool (context, udi, "battery.present", &error);

    bat->percentage    = libhal_device_get_property_int  (context, udi, "battery.charge_level.percentage", &error);

    if (G_LIKELY (libhal_device_property_exists          (context, udi, "battery.remaining_time", &error) &&
                  libhal_device_get_property_int         (context, udi, "battery.remaining_time", &error) > 0))
        bat->time      = libhal_device_get_property_int  (context, udi, "battery.remaining_time", &error);
    else
        bat->time      = 0;
}

static void
battery_add (BatteryPlugin *battery,
             const gchar   *udi)
{
    DBusError      error;
    BatteryStatus *bat;

    dbus_error_init (&error);
    
    bat = g_new0 (BatteryStatus, 1);

    bat->udi           = g_strdup (udi);
    bat->active_action = NONE;

    battery_refresh_settings (bat, udi, error);

    g_ptr_array_add (battery->batteries, bat);

    if (G_UNLIKELY (dbus_error_is_set (&error)))
    {
        DBG ("DBus Error: %s: %s", error.name, error.message);
        dbus_error_free (&error);
    }

    DBG ("Added battery: %s", udi);
}

static gboolean
battery_initilize_batteries (BatteryPlugin *battery)
{
    gint    num_devices, i;
    gchar **device_names;

    device_names = libhal_find_device_by_capability (context, "battery", &num_devices, NULL);

    if (G_UNLIKELY (device_names == NULL || num_devices == 0))
    {
        DBG ("Unable to get device list or no batteries found");
        return FALSE;
    }
    
    DBG ("%d batter%s found", num_devices, num_devices > 1 ? "ies" : "y");
    
    for (i = 0;i < num_devices;i++)
        battery_add (battery, device_names [i]);
    
    libhal_free_string_array (device_names);
    
#ifdef DUMMIES
    /* add 3 dummies for better testing */
    battery_add_dummies (battery);
#endif
    
    return TRUE;
}

void
battery_rescan_batteries (BatteryPlugin *battery)
{
    guint          i;
    DBusError      error;
    BatteryStatus *bat;

    dbus_error_init (&error);

    for (i = battery->batteries->len; i--;)
    {
        bat = g_ptr_array_index (battery->batteries, i);

        if (G_LIKELY (libhal_device_exists (context, bat->udi, &error)))
            if (G_LIKELY (libhal_device_rescan (context, bat->udi, &error)))
                battery_refresh_settings (bat, bat->udi, error);
    }
    
    if (G_UNLIKELY (dbus_error_is_set (&error)))
    {
        DBG ("DBus Error: %s: %s", error.name, error.message);
        dbus_error_free (&error);
    }
}

void
battery_stop_monitor (BatteryPlugin *battery)
{
    DBusError error;
    dbus_error_init (&error);

    if (G_LIKELY (context))
    {
        libhal_ctx_shutdown (context, &error);
        libhal_ctx_free (context);
    }
    
    if (G_LIKELY (dbus_connection))
    {
        if (G_LIKELY (dbus_connection_get_is_connected (dbus_connection)))
            dbus_connection_disconnect (dbus_connection);
        
        dbus_connection_unref (dbus_connection);
    }
    
    if (G_UNLIKELY (dbus_error_is_set (&error)))
    {
        DBG ("DBus Error: %s: %s", error.name, error.message);
        dbus_error_free (&error);
    }

    DBG ("Monitor Stopped and Freed");
}

gboolean
battery_start_monitor (BatteryPlugin *battery)
{
    DBG ("Starting the HAL monitor");

    DBusError error;
    dbus_error_init (&error);
    
    dbus_connection = dbus_bus_get (DBUS_BUS_SYSTEM, &error);
    if (G_UNLIKELY (dbus_connection == NULL))
        goto failed;
    
    /* Connect dbus to the main loop */
    dbus_connection_setup_with_g_main (dbus_connection, NULL);
    
    if (G_UNLIKELY ((context = libhal_ctx_new ()) == NULL))
        goto failed;
    
    libhal_ctx_set_user_data (context, battery);
    
    if (G_UNLIKELY (!libhal_ctx_set_dbus_connection (context, dbus_connection)))
        goto failed;
    
    if (G_UNLIKELY (!libhal_ctx_init (context, &error)))
        goto failed;
    
     /* Set callback to monitor the device changes */
    libhal_ctx_set_device_property_modified (context, hal_property_modified);
    
    if (G_UNLIKELY (battery_initilize_batteries(battery) == FALSE))
        goto nobatteries;
    
    if (G_UNLIKELY (!libhal_device_property_watch_all (context, &error)))
        goto failed;
    
    return TRUE;
    
    failed:
    
        if (dbus_error_is_set (&error))
        {
            g_warning (_("Failed to connect to the HAL daemon: %s"), error.message);
            dbus_error_free (&error);
        }
        else
        {
            g_warning (_("Failed to connect to the HAL daemon"));
        }
        
        goto free;
    
    nobatteries:
        
        g_warning (_("No batteries where found on your system"));
        
        /* fall-through */
        
    free:
        
        if (context != NULL)
        {
            libhal_ctx_free (context);
            context = NULL;
        }
        
        return FALSE;
}
