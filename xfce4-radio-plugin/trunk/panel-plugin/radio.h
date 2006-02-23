/*
 * radio plugin for Xfce4.
 *
 * Copyright (c) 2006 Stefan Ott <stefan@desire.ch>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <gtk/gtk.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <linux/videodev.h>

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

typedef enum {
	CHANGE_FREQ,
	CHANGE_PRESET
} mouse_scroll_reaction;

typedef struct {
	int			freq;
	char			name[MAX_PRESET_NAME_LENGTH];
	struct radio_preset*	next;
} radio_preset;

typedef struct {
	GtkWidget*		box;
	GtkWidget*		ebox;
	GtkWidget*		label;
	GtkWidget*		signal_bar;
	
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
} radio_gui;

static void radio_tune(radio_gui*);
