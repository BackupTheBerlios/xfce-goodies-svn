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

#include <libxfcegui4/dialogs.h>
#include <panel/global.h>
#include <panel/controls.h>
#include <panel/plugins.h>
#include <panel/xfce_support.h>

static int radio_get_signal(int fd) {
	struct video_tuner vt;
	int i, signal;

	memset(&vt,0,sizeof(vt));
	ioctl(fd, VIDIOCGTUNER, &vt);
	signal = vt.signal>>13;

	return signal;
}

static void update_signal_bar(radio_gui* data) {
	if (!data->on || !data->show_signal) {
		gtk_widget_hide(data->signal_bar);
	} else {
		gtk_widget_show(data->signal_bar);
		double signal = radio_get_signal(data->fd);
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR
					(data->signal_bar), signal / 2);
	
		GdkColor color;
		gdk_color_parse(signal > 1 ? COLOR_SIGNAL_HIGH :
						COLOR_SIGNAL_LOW, &color);
		gtk_widget_modify_bg(data->signal_bar, GTK_STATE_PRELIGHT,
								&color);
	}
}

static void update_label(radio_gui* data) {
	char* label = malloc(MAX_LABEL_LENGTH + 1);
	if (data->on) {
		sprintf(label, "%5.1f", ((float) data->freq) / 100);
	} else {
		strcpy(label, _("- off -"));
	}
	gtk_label_set_label(GTK_LABEL(data->label), label);

	free(label);
}

static gboolean radio_start(radio_gui* data) {
	struct video_tuner tuner;
	struct video_audio vid_aud;

	if (-1 == (data->fd = open(data->device, O_RDONLY))) {
		GtkWindow* win = GTK_WINDOW(gtk_widget_get_toplevel(
								data->box));
		GtkWidget* warn = gtk_message_dialog_new(win, 0,
			GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
			_("Error opening radio device"));
		gtk_dialog_run(GTK_DIALOG(warn));
		gtk_widget_destroy(warn);
		return FALSE;
	}

	if (0 == ioctl(data->fd, VIDIOCGTUNER, &tuner) &&
		(tuner.flags & VIDEO_TUNER_LOW))
		data->freqfact = 16000;

	if (ioctl(data->fd, VIDIOCSAUDIO, &vid_aud)) perror("VIDIOCGAUDIO");
	if (vid_aud.volume == 0) vid_aud.volume = 65535;
	vid_aud.flags &= ~VIDEO_AUDIO_MUTE;
	if (ioctl(data->fd, VIDIOCSAUDIO, &vid_aud)) perror("VIDIOCSAUDIO");

	radio_tune(data);
	return TRUE;
}

static void radio_stop(radio_gui* data) {
	struct video_audio vid_aud;

	if (ioctl(data->fd, VIDIOCGAUDIO, &vid_aud)) perror("VIDIOCGAUDIO");
	vid_aud.flags |= VIDEO_AUDIO_MUTE;
	if (ioctl(data->fd, VIDIOCSAUDIO, &vid_aud)) perror("VIDIOCSAUDIO");

	close(data->fd);

	if (data->show_signal) gtk_widget_hide(data->signal_bar);

	// TODO: check if blank
	xfce_exec(data->command, FALSE, FALSE, NULL);
}

static radio_preset* find_preset(const char* name, radio_gui* data) {
	radio_preset* preset = data->presets;
	while (preset != NULL) {
		if (strcmp(preset->name, name) == 0) {
			return preset;
		}
		preset = preset->next;
	}
	return NULL;
}

static gboolean append_to_presets(radio_preset* new_preset, radio_gui* data) {
	radio_preset *preset = data->presets, *prev;
	while (preset != NULL) {
		prev = preset;
		if (new_preset->freq == preset->freq) return FALSE;
		preset = preset->next;
	}
	prev->next = new_preset;
	return TRUE;
}

static void add_preset_dialog(GtkEditable* menu_item, void *pointer) {
        radio_gui* data = (radio_gui*) pointer;
	GtkWindow* win = GTK_WINDOW(gtk_widget_get_toplevel(data->box));
	GtkWidget* dialog = gtk_dialog_new_with_buttons(_("Add preset"),
				NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
				GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);
	GtkWidget* box = GTK_DIALOG(dialog)->vbox;

	GtkWidget* label = gtk_label_new(_("Station name:"));
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);

	char buf[8];
	sprintf(buf, "%5.1f FM", ((float) data->freq) / 100);
	GtkWidget* station = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(station), buf);
	gtk_widget_show(station);
	gtk_box_pack_start(GTK_BOX(box), station, FALSE, FALSE, 0);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		const char* name = gtk_entry_get_text(GTK_ENTRY(station));
		radio_preset* preset = malloc(sizeof(radio_preset));
		strncpy(preset->name, name, MAX_PRESET_NAME_LENGTH);
		preset->freq = data->freq;
		preset->next = NULL;
		if (!append_to_presets(preset, data)) {
			GtkWidget* warn = gtk_message_dialog_new(win, 0,
					GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
			_("There is already a preset with this frequency."));
			gtk_dialog_run(GTK_DIALOG(warn));
			gtk_widget_destroy(warn);
		}
	}
	gtk_widget_destroy(dialog);
}

static void select_preset(GtkEditable* menu_item, void *pointer) {
        radio_gui* data = (radio_gui*) pointer;
	GtkWidget* label = gtk_bin_get_child(GTK_BIN(menu_item));
	const char* text = gtk_label_get_text(GTK_LABEL(label));
	radio_preset* preset = find_preset(text, data);
	data->freq = preset->freq;
	radio_tune(data);
}

static gboolean mouse_click(GtkWidget* src, GdkEventButton *event, radio_gui*
									data) {
	if (event->button == 1) {
		if (!data->on) {
			data->on = radio_start(data);
		} else {
			data->on = FALSE;
			radio_stop(data);
		}
	} else if (event->button == 2 && data->on) {
		GtkWidget* menu = gtk_menu_new();
		GtkWidget* item = gtk_menu_item_new_with_label(_("Presets"));
		gtk_widget_show(item);
		gtk_menu_append(menu, item);
		gtk_widget_set_sensitive(item, FALSE);

		GtkWidget* separator = gtk_separator_menu_item_new();
		gtk_widget_show(separator);
		gtk_container_add(GTK_CONTAINER (menu), separator);
		gtk_widget_set_sensitive(separator, FALSE);

		radio_preset* preset = data->presets;
		while (preset != NULL) {
			item = gtk_menu_item_new_with_label(preset->name);
			gtk_widget_show(item);
			gtk_menu_append(menu, item);
			g_signal_connect(GTK_WIDGET(item), "activate",
					G_CALLBACK(select_preset), data);
			preset = preset->next;
		}

		separator = gtk_separator_menu_item_new();
		gtk_widget_show(separator);
		gtk_container_add(GTK_CONTAINER (menu), separator);
		gtk_widget_set_sensitive(separator, FALSE);

		item = gtk_menu_item_new_with_label(_("Add preset"));
		gtk_widget_show(item);
		gtk_menu_append(menu, item);
		g_signal_connect(GTK_WIDGET(item), "activate",
					G_CALLBACK(add_preset_dialog), data);

		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 
				event->button, event->time);
	}
	update_label(data);
	update_signal_bar(data);
	return event->button != 3;
}

static void radio_tune(radio_gui* data) {
	int freq = (data->freq * data->freqfact) / 100;
	ioctl(data->fd, VIDIOCSFREQ, &freq);

	update_label(data);
	update_signal_bar(data);
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

	gui->signal_bar = gtk_progress_bar_new();
	gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(gui->signal_bar),
						GTK_PROGRESS_LEFT_TO_RIGHT);

	gui->box = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(gui->box);

	gui->label = gtk_label_new("");
	gtk_widget_show(gui->label);

	gtk_box_pack_start(GTK_BOX(gui->box), gui->label, FALSE, FALSE,0);
	gtk_box_pack_start(GTK_BOX(gui->box), gui->signal_bar, FALSE, FALSE,0);

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
	plugin_data->show_signal = FALSE;
	strcpy(plugin_data->device, "/dev/radio0");

	// HACK
	radio_preset* SP = malloc(sizeof(radio_preset));
	strncpy(SP->name, "SwissPop", MAX_PRESET_NAME_LENGTH);
	SP->freq = 10700;
	SP->next = NULL;

	radio_preset* BE1 = malloc(sizeof(radio_preset));
	strncpy(BE1->name, "BE1", MAX_PRESET_NAME_LENGTH);
	BE1->freq = 9700;
	BE1->next = SP;

	radio_preset* VIRUS = malloc(sizeof(radio_preset));
	strncpy(VIRUS->name, "Virus", MAX_PRESET_NAME_LENGTH);
	VIRUS->freq = 9430;
	VIRUS->next = BE1;

	plugin_data->presets = VIRUS;
	// END HACK
	
	update_label(plugin_data);

	gtk_container_add(GTK_CONTAINER(ctrl->base), plugin_data->ebox);

	return TRUE;
}

void radio_command_changed(GtkEditable* editable, void *pointer) {
	radio_gui* data = (radio_gui*) pointer;
	const char* command = gtk_entry_get_text(GTK_ENTRY(editable));
	strncpy(data->command, command, MAX_COMMAND_LENGTH);
}

void radio_device_changed(GtkEditable* editable, void *pointer) {
	radio_gui* data = (radio_gui*) pointer;
	const char* device = gtk_entry_get_text(GTK_ENTRY(editable));
	strncpy(data->device, device, MAX_DEVICE_NAME_LENGTH);
}

void radio_show_signal_changed(GtkEditable* editable, void *pointer) {
	radio_gui* data = (radio_gui*) pointer;
	data->show_signal = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
								(editable));
	update_signal_bar(data);
}

static void plugin_create_options(Control *ctrl, GtkContainer *container,
							GtkWidget *done) {
	radio_gui* data = ctrl->data;

	GtkWidget *table1;
	GtkWidget *deviceLabel;
	GtkWidget *qualityLabel;
	GtkWidget *hbox1;
	GSList *showSignal_group = NULL;
	GtkWidget *radioShowSignal;
	GtkWidget *radioHideSignal;
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

	radioShowSignal = gtk_radio_button_new_with_mnemonic (NULL, _("yes"));
	gtk_widget_show (radioShowSignal);
	gtk_box_pack_start (GTK_BOX (hbox1), radioShowSignal, FALSE, FALSE, 0);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (radioShowSignal), 
							showSignal_group);
	showSignal_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON
							(radioShowSignal));

	radioHideSignal = gtk_radio_button_new_with_mnemonic (NULL, _("no"));
	gtk_widget_show (radioHideSignal);

	gtk_box_pack_start (GTK_BOX (hbox1), radioHideSignal, FALSE, FALSE, 0);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (radioHideSignal), 
							showSignal_group);
	showSignal_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON 
							(radioHideSignal));
	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radioShowSignal),
							data->show_signal);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radioHideSignal),
							!data->show_signal);

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
	g_signal_connect (G_OBJECT (radioShowSignal), "toggled",
				G_CALLBACK (radio_show_signal_changed), data);
}

static void plugin_write_config(Control *ctrl, xmlNodePtr parent) {
	radio_gui* data = ctrl->data;
	char buf[32];
	xmlNodePtr xml;

	xml = xmlNewTextChild(parent, NULL, "xfce4-radio", NULL);

	snprintf(buf, 32, "%d", data->freq);
	xmlSetProp(xml, "freq", buf);

	xmlSetProp(xml, "dev", data->device);
	xmlSetProp(xml, "cmd", data->command);

	snprintf(buf, 2, "%i", data->show_signal);
	xmlSetProp(xml, "show_signal", buf);
}

static void plugin_read_config(Control *ctrl, xmlNodePtr parent) {
	xmlChar* value;
	xmlNodePtr child;

	radio_gui* data = ctrl->data;

	if (!parent || !parent->children) return;

	for (child = parent->children; child; child = child->next) {
		if (!(xmlStrEqual (child->name, "xfce4-radio"))) continue;

		if ((value = xmlGetProp(child, (const xmlChar*) "freq")) !=
									NULL) {
			data->freq = atoi(value);
			g_free(value);
		}
		if ((value = xmlGetProp(child, (const xmlChar*) "dev")) !=
									NULL) {
			strcpy(data->device, value);
			g_free(value);
		}
		if ((value = xmlGetProp(child, (const xmlChar*) "cmd")) !=
									NULL) {
			strcpy(data->command, value);
			g_free(value);
		}
		if ((value = xmlGetProp(child, (const xmlChar*) "show_signal"))
								!= NULL) {
			data->show_signal = atoi(value);
			g_free(value);
		}
	}
}

static void plugin_set_size(Control *ctrl, int size) {
	radio_gui* data = ctrl->data;

	int icon = icon_size[size];

	gtk_widget_set_size_request(data->signal_bar, icon - 6, 2 + 2 * size);
}

static void plugin_attach_callback(Control *ctrl, const gchar *signal,
						GCallback cb, gpointer data) {}

G_MODULE_EXPORT void xfce_control_class_init(ControlClass *cc) {
	cc->name = "radio";
	cc->caption = _("Radio player");

	cc->create_control = plugin_control_new;
	cc->free = plugin_free;
	cc->read_config = plugin_read_config;
	cc->write_config = plugin_write_config;
	cc->attach_callback = plugin_attach_callback;
	cc->set_size = plugin_set_size;
	cc->create_options = plugin_create_options;
}

XFCE_PLUGIN_CHECK_INIT
