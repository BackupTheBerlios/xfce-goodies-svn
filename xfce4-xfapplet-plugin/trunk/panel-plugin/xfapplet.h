/*
 *  xfce4-xfapplet-plugin - a gnome applet displaying plugin for xfce4 panel
 *  Copyright (c) 2006 Adriano Winter Bess <awbess@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License ONLY.
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

#ifndef XFAPPLET_PLUGIN_H
#define XFAPPLET_PLUGIN_H

#include <gtk/gtk.h>
#include <libxfce4panel/xfce-panel-plugin.h>

typedef struct  {
	XfcePanelPlugin   *plugin;
	gchar             *iid;
	gchar             *gconf_key;
	GtkWidget         *tv;
	GSList            *applets;
} XfAppletPlugin;

typedef struct {
	gchar     *name;
	gchar     *description;
	GdkPixbuf *icon;
	gchar     *iid;	
} GnomeAppletInfo;

void          xfapplet_chooser_dialog (XfcePanelPlugin*, XfAppletPlugin*);

void          xfapplet_setup_full     (XfAppletPlugin*);

#endif /* XFAPPLET_PLUGIN_H */