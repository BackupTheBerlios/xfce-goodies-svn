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

#ifndef _BATTERY_H
#define _BATTERY_H

G_BEGIN_DECLS

typedef enum
{
    NOTHING = 0,
    MESSAGE,
    COMMAND,
    TERMINAL,
}
Action;

typedef enum
{
    NONE = 0,
    CHARGED,
    LOW,
    CRITICAL,
}
ActionType;

typedef struct
{
    gchar         *udi; /* /org/freedesktop/Hal/devices/acpi_BAT0 */
    
    gboolean       charging;
    gboolean       present;

    ActionType     active_action;

    gint           time;
    guint          percentage;
}
BatteryStatus;

typedef struct
{
    XfcePanelPlugin   *plugin;
    gboolean           running;

    /* Array with all the batteries */
    GPtrArray         *batteries;

    /* Widgets */
    GtkWidget         *ebox, *icon, *progressbar, *label;
    GtkTooltips       *tooltip;

    /* Widget Settings */
    gchar             *iconname;
    guint              show_battery;

    /* Settings: Appearance */
    gboolean           show_icon, show_progressbar, show_percentage, show_time;
    gboolean           tip_time;

    /* Settings: Actions */
    guint              perc_critical, perc_low;
    Action             action_critical, action_low, action_charged;
    gchar             *command_critical, *command_low, *command_charged;
}
BatteryPlugin;

gchar *
battery_icon_name     (BatteryStatus *bat);

void
battery_widgets       (BatteryPlugin *battery);

void
battery_update_plugin (BatteryPlugin *battery);

void
battery_save          (XfcePanelPlugin *plugin, BatteryPlugin *battery);

G_END_DECLS

#endif /* _BATTERY_H */
