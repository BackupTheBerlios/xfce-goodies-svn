/*
 * radio plugin for Xfce4.
 *
 * Copyright 2006 Stefan Ott, All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 
 */

#include <gtk/gtk.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <linux/videodev.h>

#include <libxfce4panel/xfce-panel-plugin.h>

#define FREQ_MIN		8750
#define FREQ_MAX		10800
#define FREQ_INIT		10795
#define FREQ_STEP		10

#define MAX_LABEL_LENGTH	7
#define MAX_DEVICE_NAME_LENGTH	32
#define MAX_COMMAND_LENGTH	128
#define MAX_PRESET_NAME_LENGTH	15

#define COLOR_SIGNAL_LOW	"#ffff00"
#define COLOR_SIGNAL_HIGH	"#00ff00"

#define BORDER 8

typedef enum {
	CHANGE_FREQ,
	CHANGE_PRESET
} mouse_scroll_reaction;

struct radio_preset_st {
	int			freq;
	char			name[MAX_PRESET_NAME_LENGTH];
	struct radio_preset_st*	next;
};

typedef struct radio_preset_st radio_preset;

typedef struct {
	GtkWidget*		box;
	GtkWidget*		ebox;
	GtkWidget*		label;
	GtkWidget*		signal_bar;
	GtkTooltips*		tooltips;
	
	int			timeout_id;

	gboolean		on;
	gboolean		show_signal;
	int			freq;
	int			fd;
	int			freqfact;
	char			device[MAX_DEVICE_NAME_LENGTH];
	char			command[MAX_COMMAND_LENGTH];
	radio_preset*		presets;
	mouse_scroll_reaction	scroll;

	XfcePanelPlugin		*plugin;
} radio_gui;

static void radio_tune (radio_gui*);
static radio_preset* find_preset_by_freq (int, radio_gui*);
static void read_config (XfcePanelPlugin *, radio_gui *);
static void write_config (XfcePanelPlugin *, radio_gui *);
