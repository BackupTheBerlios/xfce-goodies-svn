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

#include <ctype.h>

#include "radio.h"

#include <gtk/gtk.h>

#include <libxfcegui4/dialogs.h>
#include <panel/global.h>
#include <panel/controls.h>
#include <panel/plugins.h>
#include <panel/xfce_support.h>

static void update_label(radio_gui* data) {
	char* label = malloc(MAX_LABEL_LENGTH + 1);
	if (data->on) {
		sprintf(label, "%5.2f", ((float) data->freq) / 100);
	} else {
		strcpy(label, "- off -");
	}
	gtk_label_set_label(GTK_LABEL(data->label), label);
	free(label);
}

static void radio_start(radio_gui* data) {
	struct video_tuner tuner;
	struct video_audio vid_aud;

	// TODO: report errors
	data->fd = open(data->device, O_RDONLY);

	if (0 == ioctl(data->fd, VIDIOCGTUNER, &tuner) &&
		(tuner.flags & VIDEO_TUNER_LOW))
		data->freqfact = 16000;

	ioctl(data->fd, VIDIOCSAUDIO, &vid_aud);
	vid_aud.volume = 65535;
	vid_aud.flags &= ~VIDEO_AUDIO_MUTE;
	ioctl(data->fd, VIDIOCSAUDIO, &vid_aud);

	radio_tune(data);
}

static void radio_stop(radio_gui* data) {
	close(data->fd);

	// TODO: check if blank
	g_printf("%s\n", data->command);
	xfce_exec(data->command, FALSE, FALSE, NULL);
}

static gboolean mouse_click(GtkWidget* src, GdkEventButton *event, radio_gui*
									data) {
	if (event->button == 1) {
		data->on = !data->on;
		if (data->on) {
			radio_start(data);
		} else {
			radio_stop(data);
		}
	}
	update_label(data);
	return event->button != 3;
}

static void radio_tune(radio_gui* data) {
	int freq = (data->freq * data->freqfact) / 100;
	ioctl(data->fd, VIDIOCSFREQ, &freq);
	update_label(data);
}

static void mouse_scroll(GtkWidget* src, GdkEventScroll *event, radio_gui* 
									data) {
	if (data->on) {
		int direction = event->direction == GDK_SCROLL_UP ? -1 : 1;
		data->freq += direction * FREQ_STEP;
		if (data->freq > FREQ_MAX) data->freq = FREQ_MIN;
		if (data->freq < FREQ_MIN) data->freq = FREQ_MAX;
		radio_tune(data);
	}
}

static radio_gui* create_gui() {
	radio_gui* gui;
	gui = g_new(radio_gui, 1);

	gui->ebox = gtk_event_box_new();
	gtk_widget_show(gui->ebox);
	g_signal_connect(GTK_WIDGET(gui->ebox), "button_press_event",
						G_CALLBACK(mouse_click), gui);
	g_signal_connect(GTK_WIDGET(gui->ebox), "scroll_event", 
						G_CALLBACK(mouse_scroll), gui);

	gui->box = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(gui->box);

	gui->label = gtk_label_new("");
	gtk_widget_show(gui->label);
	gtk_container_add(GTK_CONTAINER(gui->box), gui->label);

	gtk_container_add(GTK_CONTAINER(gui->ebox), gui->box);
	
	return gui;
}

static void plugin_free(Control *ctrl) {
	g_return_if_fail(ctrl != NULL);
	g_return_if_fail(ctrl->data != NULL);

	radio_gui* gui = ctrl->data;

	if (gui->on) radio_stop(gui);

	g_free(gui);
}

static gboolean plugin_control_new(Control *ctrl) {
	radio_gui* plugin_data = create_gui();
	ctrl->data = (gpointer) plugin_data;
	
	plugin_data->on = FALSE;
	plugin_data->freq = FREQ_INIT;
	plugin_data->freqfact = 16;
	strcpy(plugin_data->device, "/dev/radio0");
	strcpy(plugin_data->command, "/home/stefan/muteradio.sh");

	update_label(plugin_data);

	gtk_container_add(GTK_CONTAINER(ctrl->base), plugin_data->ebox);

	return TRUE;
}

void radio_command_changed(GtkEditable* editable, void *pointer) {
	radio_gui* data = (radio_gui*) pointer;
	char* command = gtk_entry_get_text(GTK_ENTRY(editable));
	strncpy(data->command, command, MAX_COMMAND_LENGTH);
}

void radio_device_changed(GtkEditable* editable, void *pointer) {
	radio_gui* data = (radio_gui*) pointer;
	char* device = gtk_entry_get_text(GTK_ENTRY(editable));
	strncpy(data->device, device, MAX_DEVICE_NAME_LENGTH);
}

static void radio_create_options(Control *ctrl, GtkContainer *container,
							GtkWidget *done) {
	radio_gui* data = ctrl->data;

	GtkWidget *table1;
	GtkWidget *deviceLabel;
	GtkWidget *qualityLabel;
	GtkWidget *hbox1;
	GSList *showSignal_group = NULL;
	GtkWidget *showSignal;
	GtkWidget *executeLabel;
	GtkWidget *command;
	GtkWidget *device;

	table1 = gtk_table_new (3, 2, FALSE);
	gtk_widget_show (table1);
	gtk_container_add (GTK_CONTAINER (container), table1);

	deviceLabel = gtk_label_new (_("V4L device"));
	gtk_widget_show (deviceLabel);
	gtk_table_attach (GTK_TABLE (table1), deviceLabel, 0, 1, 0, 1,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (deviceLabel), 0, 0.5);

	qualityLabel = gtk_label_new (_("Display signal strength"));
	gtk_widget_show (qualityLabel);
	gtk_table_attach (GTK_TABLE (table1), qualityLabel, 0, 1, 1, 2,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (qualityLabel), 0, 0.5);

	hbox1 = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox1);
	gtk_table_attach (GTK_TABLE (table1), hbox1, 1, 2, 1, 2,
	(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 0, 0);

	showSignal = gtk_radio_button_new_with_mnemonic (NULL, _("yes"));
	gtk_widget_show (showSignal);
	gtk_box_pack_start (GTK_BOX (hbox1), showSignal, FALSE, FALSE, 0);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (showSignal), 
							showSignal_group);
	showSignal_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON
								(showSignal));

	showSignal = gtk_radio_button_new_with_mnemonic (NULL, _("no"));
	gtk_widget_show (showSignal);

	gtk_box_pack_start (GTK_BOX (hbox1), showSignal, FALSE, FALSE, 0);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (showSignal), 
							showSignal_group);
	showSignal_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON 
								(showSignal));

	executeLabel = gtk_label_new (_("Execute command after sutdown"));
	gtk_widget_show (executeLabel);
	gtk_table_attach (GTK_TABLE (table1), executeLabel, 0, 1, 2, 3,
		(GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
        gtk_misc_set_alignment (GTK_MISC (executeLabel), 0, 0.5);

	command = gtk_entry_new ();
	gtk_entry_set_text(GTK_ENTRY(command), data->command);
	gtk_widget_show (command);
	gtk_table_attach (GTK_TABLE (table1), command, 1, 2, 2, 3,
				(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
				(GtkAttachOptions) (0), 0, 0);

	device = gtk_entry_new ();
	gtk_entry_set_text(GTK_ENTRY(device), data->device);
	gtk_widget_show (device);
	gtk_table_attach (GTK_TABLE (table1), device, 1, 2, 0, 1,
				(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
				(GtkAttachOptions) (0), 0, 0);

	g_signal_connect ((gpointer) command, "changed",
				G_CALLBACK (radio_command_changed), data);
	g_signal_connect ((gpointer) device, "changed",
				G_CALLBACK (radio_device_changed), data);
}



static void radio_read_config(Control *ctrl, xmlNodePtr parent) { }
static void radio_write_config(Control *ctrl, xmlNodePtr parent) { }
static void radio_set_size(Control *ctrl, int size) { }
static void radio_attach_callback(Control *ctrl, const gchar *signal,
						GCallback cb, gpointer data) {}

G_MODULE_EXPORT void xfce_control_class_init(ControlClass *cc) {
	cc->name = "radio";
	cc->caption = _("Radio player");

	cc->create_control = plugin_control_new;
	cc->free = plugin_free;
	cc->read_config = radio_read_config;
	cc->write_config = radio_write_config;
	cc->attach_callback = radio_attach_callback;
	cc->set_size = radio_set_size;
	cc->create_options = radio_create_options;
}

XFCE_PLUGIN_CHECK_INIT
