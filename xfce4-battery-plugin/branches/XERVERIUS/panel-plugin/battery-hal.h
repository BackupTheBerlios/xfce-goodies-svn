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

#ifndef _BATTERY_HAL_H
#define _BATTERY_HAL_H

G_BEGIN_DECLS

const gchar *
battery_get_property_string (const gchar *udi, const gchar *key);

gint
battery_get_property_int    (const gchar *udi, const gchar *key);

void
battery_remove              (BatteryStatus *bat);

void
battery_rescan_batteries    (BatteryPlugin *battery);

gboolean
battery_start_monitor       (BatteryPlugin *battery);

void
battery_stop_monitor        (BatteryPlugin *battery);

G_END_DECLS

#endif /* _BATTERY_HAL_H */
