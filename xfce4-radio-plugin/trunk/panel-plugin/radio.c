/*
 * radio plugin for Xfce4.
 *
 * Copyright (c) 2006 Stefan Ott, All rights reserved.
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

#include <ctype.h>
#include <string.h>

#include "radio.h"

#include <libxfcegui4/dialogs.h>

static int
radio_get_signal (int fd)
{
	struct video_tuner vt;
	int i, signal;

	memset (&vt, 0, sizeof (vt));
	ioctl (fd, VIDIOCGTUNER, &vt);
	signal = vt.signal >> 13;

	return signal;
}

static gboolean
update_signal_bar (radio_gui* data)
{
	if (!data->on || !data->show_signal) {
		gtk_widget_hide (data->signal_bar);
		data->timeout_id = 0;
		return FALSE;
	} else {
		gtk_widget_show (data->signal_bar);
		double signal = radio_get_signal (data->fd);
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR
					(data->signal_bar), signal / 2);
	
		GdkColor color;
		gdk_color_parse (signal > 1 ? COLOR_SIGNAL_HIGH :
						COLOR_SIGNAL_LOW, &color);
		gtk_widget_modify_bg (data->signal_bar, GTK_STATE_PRELIGHT,
								&color);
		if (data->timeout_id == 0) {
			data->timeout_id = g_timeout_add(500, (GtkFunction)
					update_signal_bar, (gpointer) data);
		}
		return TRUE;
	}
}

static void
update_tooltip (radio_gui* data)
{
	GtkWidget* ebox = data->ebox;

	char *text = malloc(1024);
	strcpy (text, _("Not tuned"));
	radio_preset* preset = find_preset_by_freq (data->freq, data);

	if (preset) {
		sprintf(text, "Tuned to %s", preset->name);
	} else {
		sprintf(text, "Tuned to %5.1f", (float) data->freq / 100);
	}
	gtk_tooltips_set_tip (data->tooltips, ebox, text, NULL);
}

static void
update_label (radio_gui* data)
{
	char *label = malloc (MAX_LABEL_LENGTH + 1);
	if (data->on) {
		sprintf (label, "%5.1f", ((float) data->freq) / 100);
		update_tooltip (data);
	} else {
		strcpy (label, _("- off -"));
	}
	gtk_label_set_label (GTK_LABEL (data->label), label);

	free (label);
}

static gboolean
radio_start (radio_gui* data)
{
	struct video_tuner tuner;
	struct video_audio vid_aud;

	if (-1 == (data->fd = open (data->device, O_RDONLY))) {
		GtkWindow* win = GTK_WINDOW (gtk_widget_get_toplevel(
								data->box));
		GtkWidget* warn = gtk_message_dialog_new (win, 0,
			GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
			_("Error opening radio device"));
		gtk_dialog_run (GTK_DIALOG (warn));
		gtk_widget_destroy (warn);
		return FALSE;
	}

	if (0 == ioctl (data->fd, VIDIOCGTUNER, &tuner) &&
		(tuner.flags & VIDEO_TUNER_LOW))
		data->freqfact = 16000;

	if (ioctl (data->fd, VIDIOCSAUDIO, &vid_aud)) perror("VIDIOCGAUDIO");
	if (vid_aud.volume == 0) vid_aud.volume = 65535;
	vid_aud.flags &= ~VIDEO_AUDIO_MUTE;
	if (ioctl (data->fd, VIDIOCSAUDIO, &vid_aud)) perror("VIDIOCSAUDIO");

	radio_tune (data);
	gtk_tooltips_enable (data->tooltips);
	return TRUE;
}

static void
radio_stop (radio_gui* data)
{
	struct video_audio vid_aud;
	gtk_tooltips_disable (data->tooltips);

	if (ioctl (data->fd, VIDIOCGAUDIO, &vid_aud)) perror("VIDIOCGAUDIO");
	vid_aud.flags |= VIDEO_AUDIO_MUTE;
	if (ioctl (data->fd, VIDIOCSAUDIO, &vid_aud)) perror("VIDIOCSAUDIO");

	close (data->fd);

	if (data->show_signal) gtk_widget_hide (data->signal_bar);

	if (strcmp (data->command, "") != 0) {
		xfce_exec (data->command, FALSE, FALSE, NULL);
	}
}

#define make_preset_finder(name, arg, comparison)			\
	static radio_preset*						\
	find_preset_by_##name (arg, radio_gui* data)			\
	{								\
		radio_preset* preset = data->presets;			\
		while (preset != NULL) {				\
			if (comparison) {				\
				return preset;				\
			}						\
			preset = preset->next;				\
		}							\
		return NULL;						\
	}

make_preset_finder(name, const char* name, strcmp (preset->name, name) == 0);
make_preset_finder(freq, int freq, preset->freq == freq);

static gboolean
add_before (radio_preset* a, radio_preset* b)
{
	return (b == NULL || strcmp (a->name, b->name) < 0);
}

static gboolean
append_to_presets (radio_preset* new_preset, radio_gui* data)
{
	radio_preset *preset = data->presets, *prev;

	if (find_preset_by_freq (new_preset->freq, data)) return FALSE;

	if (data->presets == NULL) {
		data->presets = new_preset;
		return TRUE;
	} else if (add_before (new_preset, data->presets)) {
		new_preset->next = data->presets;
		data->presets = new_preset;
		return TRUE;
	} else {
		while (preset != NULL) {
			prev = preset;

			preset = preset->next;
			if (add_before (new_preset, preset)) {
				new_preset->next = preset;
				prev->next = new_preset;
				return TRUE;
			}
		}
	}
	return FALSE;
}

static radio_preset *
pop_preset (radio_preset *target, radio_gui *data)
{
	radio_preset *preset = data->presets, *prev;

	while (preset != NULL) {
		if (preset->freq == target->freq) {
			if (preset == data->presets) {
				data->presets = preset->next;
			} else {
				prev->next = preset->next;
			}
			return preset;
		} else {
			prev = preset;
		}
		preset = preset->next;
	}
	return NULL;
}

static void
rename_preset (GtkEditable* menu_item, void *pointer)
{
	radio_gui* data = (radio_gui*) pointer;
	radio_preset* preset = find_preset_by_freq (data->freq, data);
	if (!preset) return;
	GtkWidget* dialog = gtk_dialog_new_with_buttons(_("Add preset"),
				NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_STOCK_OK, GTK_RESPONSE_OK,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	GtkWidget* box = GTK_DIALOG (dialog)->vbox;

	GtkWidget* label = gtk_label_new(_("Station name:"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);

	GtkWidget* station = gtk_entry_new_with_max_length (
						MAX_PRESET_NAME_LENGTH);
	gtk_entry_set_text (GTK_ENTRY (station), preset->name);
	gtk_widget_show (station);
	gtk_box_pack_start (GTK_BOX (box), station, FALSE, FALSE, 0);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK) {
		strncpy (preset->name, gtk_entry_get_text (GTK_ENTRY (station)),
						MAX_PRESET_NAME_LENGTH);
		pop_preset (preset, data);
		append_to_presets (preset, data);
	}
	gtk_widget_destroy (dialog);
	update_tooltip (data);
}

static void
add_preset_dialog (GtkEditable* menu_item, void *pointer)
{
        radio_gui* data = (radio_gui*) pointer;
	GtkWindow* win = GTK_WINDOW (gtk_widget_get_toplevel (data->box));
	GtkWidget* dialog = gtk_dialog_new_with_buttons(_("Add preset"),
				NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
				GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);
	GtkWidget* box = GTK_DIALOG (dialog)->vbox;

	GtkWidget* label = gtk_label_new (_("Station name:"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);

	char buf[8];
	sprintf (buf, "%5.1f FM", ((float) data->freq) / 100);
	GtkWidget* station = gtk_entry_new_with_max_length (
						MAX_PRESET_NAME_LENGTH);
	gtk_entry_set_text (GTK_ENTRY (station), buf);
	gtk_widget_show (station);
	gtk_box_pack_start (GTK_BOX (box), station, FALSE, FALSE, 0);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		const char* name = gtk_entry_get_text (GTK_ENTRY (station));
		radio_preset* preset = malloc (sizeof (radio_preset));
		strncpy (preset->name, name, MAX_PRESET_NAME_LENGTH);
		preset->freq = data->freq;
		preset->next = NULL;
		if (!append_to_presets (preset, data)) {
			GtkWidget* warn = gtk_message_dialog_new(win, 0,
					GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
			_("There is already a preset with this frequency."));
			gtk_dialog_run (GTK_DIALOG (warn));
			gtk_widget_destroy (warn);
		} else {
			write_config (data->plugin, data);
		}
	}
	gtk_widget_destroy (dialog);
}

static gboolean
parse_freq_and_tune (const char* freq_char, radio_gui* data)
{
	int freq_int = 100 * atoi (freq_char);

	char* decimals = strstr (freq_char, ".");
	if (!decimals) {
		decimals = "0";
	} else {
		decimals++;
	}
	int decimal_int = atoi (decimals);
	if (decimal_int > 10) return FALSE;
	freq_int += 10 * decimal_int;

	if (freq_int >= FREQ_MIN && freq_int <= FREQ_MAX) {
		data->freq = freq_int;
		radio_tune (data);
		return TRUE;
	}
	return FALSE;
}

static void
radio_tune_gui (GtkEditable *menu_item, void *pointer)
{
        radio_gui* data = (radio_gui*) pointer;
	GtkWindow* win = GTK_WINDOW (gtk_widget_get_toplevel (data->box));
	GtkWidget* dialog = gtk_dialog_new_with_buttons (_("Tune radio"),
				NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_STOCK_OK, GTK_RESPONSE_OK,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	GtkWidget* box = GTK_DIALOG (dialog)->vbox;

	GtkWidget* label = gtk_label_new (_("Frequency [MHz]:"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);

	GtkWidget* freq = gtk_entry_new_with_max_length (5);
	gtk_widget_show (freq);
	gtk_box_pack_start (GTK_BOX (box), freq, FALSE, FALSE, 0);

	int retval;
	for (;;) {
		retval = gtk_dialog_run (GTK_DIALOG (dialog));

		if (	retval == GTK_RESPONSE_CANCEL || 
			retval == GTK_RESPONSE_DELETE_EVENT ||
			retval == GTK_RESPONSE_NONE) {
				break;
		}

		const char* freq_char = gtk_entry_get_text (GTK_ENTRY (freq));
		if (parse_freq_and_tune (freq_char, data)) break;
	
		GtkWidget* warn = gtk_message_dialog_new (win, 0,
					GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
						_("Illegal frequency."));
		gtk_dialog_run (GTK_DIALOG (warn));
		gtk_widget_destroy (warn);
	}
	gtk_widget_destroy (dialog);
}

static void
remove_preset (GtkEditable* menu_item, void *pointer)
{
	radio_gui* data = (radio_gui*) pointer;
	radio_preset *preset;

	preset = pop_preset (find_preset_by_freq (data->freq, data), data);
	free (preset);
}

static void
select_preset (GtkEditable* menu_item, void *pointer)
{
        radio_gui* data = (radio_gui*) pointer;
	GtkWidget* label = gtk_bin_get_child (GTK_BIN (menu_item));
	const char* text = gtk_label_get_text (GTK_LABEL (label));
	radio_preset* preset = find_preset_by_name (text, data);
	data->freq = preset->freq;
	radio_tune (data);
}

static gboolean
mouse_click (GtkWidget* src, GdkEventButton *event, radio_gui* data)
{
	if (event->button == 1) {
		if (!data->on) {
			data->on = radio_start (data);
		} else {
			data->on = FALSE;
			radio_stop (data);
		}
	} else if (event->button == 2 && data->on) {
		GtkWidget* menu = gtk_menu_new ();
		GtkWidget* item = gtk_menu_item_new_with_label (_("Presets"));
		gtk_widget_show (item);
		gtk_menu_append (menu, item);
		gtk_widget_set_sensitive (item, FALSE);

		GtkWidget* separator = gtk_separator_menu_item_new ();
		gtk_widget_show (separator);
		gtk_container_add (GTK_CONTAINER (menu), separator);
		gtk_widget_set_sensitive (separator, FALSE);

		radio_preset* preset = data->presets;
		while (preset != NULL) {
			item = gtk_menu_item_new_with_label (preset->name);
			gtk_widget_show (item);
			gtk_menu_append (menu, item);
			g_signal_connect (GTK_WIDGET (item), "activate",
					G_CALLBACK (select_preset), data);
			gtk_widget_set_sensitive (item, preset->freq !=
								data->freq);
			preset = preset->next;
		}

		separator = gtk_separator_menu_item_new ();
		gtk_widget_show (separator);
		gtk_container_add (GTK_CONTAINER (menu), separator);
		gtk_widget_set_sensitive (separator, FALSE);

		radio_preset* current = find_preset_by_freq (data->freq, data);

		item = gtk_menu_item_new_with_label (_("Add preset"));
		gtk_widget_show (item);
		gtk_menu_append (menu, item);
		g_signal_connect (GTK_WIDGET (item), "activate",
					G_CALLBACK (add_preset_dialog), data);
		gtk_widget_set_sensitive (item, current == NULL);

		item = gtk_menu_item_new_with_label (_("Delete active preset"));
		gtk_widget_show (item);
		gtk_menu_append (menu, item);
		g_signal_connect (GTK_WIDGET (item), "activate",
					G_CALLBACK (remove_preset), data);
		gtk_widget_set_sensitive (item, current != NULL);

		item = gtk_menu_item_new_with_label (_("Rename active preset"));
		gtk_widget_show (item);
		gtk_menu_append (menu, item);
		g_signal_connect (GTK_WIDGET (item), "activate",
					G_CALLBACK (rename_preset), data);
		gtk_widget_set_sensitive (item, current != NULL);

		separator = gtk_separator_menu_item_new ();
		gtk_widget_show (separator);
		gtk_container_add (GTK_CONTAINER (menu), separator);
		gtk_widget_set_sensitive (separator, FALSE);

		item = gtk_menu_item_new_with_label (_("Tune to frequency"));
		gtk_widget_show (item);
		gtk_menu_append (menu, item);
		g_signal_connect (GTK_WIDGET (item), "activate",
					G_CALLBACK (radio_tune_gui), data);

		gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 
				event->button, event->time);
	}
	update_label (data);
	update_signal_bar (data);
	return event->button != 3;
}

static void
radio_tune (radio_gui* data)
{
	int freq = (data->freq * data->freqfact) / 100;
	ioctl (data->fd, VIDIOCSFREQ, &freq);

	update_label (data);
	update_signal_bar (data);

	write_config (data->plugin, data);
}

static void
tune_to_prev_preset (radio_gui* data)
{
	radio_preset *preset = data->presets, *prev;

	if (preset->freq == data->freq) {
		while (preset->next != NULL) {
			preset = preset->next;
		}
		data->freq = preset->freq;
	} else {
		prev = preset;
		while (preset != NULL) {
			if (preset->freq == data->freq) {
				data->freq = prev->freq;
			}
			prev = preset;
			preset = preset->next;
		}
	}
	radio_tune (data);
}

static void
mouse_scroll(GtkWidget* src, GdkEventScroll *event, radio_gui* data)
{
	if (data->on) {
		int direction = event->direction == GDK_SCROLL_UP ? -1 : 1;
		if (data->scroll == CHANGE_FREQ) {
			data->freq += direction * FREQ_STEP;
			if (data->freq > FREQ_MAX) data->freq = FREQ_MIN;
			if (data->freq < FREQ_MIN) data->freq = FREQ_MAX;
			radio_tune (data);
		} else if (data->scroll == CHANGE_PRESET) {
			radio_preset* preset = find_preset_by_freq
							(data->freq, data);
			if (preset) {
				// preset found
				if (direction == 1) {
					// tune to next preset
					preset = preset->next == NULL ?
						data->presets :	preset->next;
					data->freq = preset->freq;
					radio_tune (data);
				} else {
					tune_to_prev_preset (data);
				}
			} else {
				// tune to first preset, if it exists
				preset = data->presets;
				if (preset) {
					data->freq = preset->freq;
					radio_tune (data);
				}
			}
		}
	}
	// TODO: remove the following line
	read_config (data->plugin, data);
}

static radio_gui *
create_gui ()
{
	radio_gui* gui;
	gui = g_new (radio_gui, 1);

	gui->ebox = gtk_event_box_new ();
	gtk_widget_show (gui->ebox);
	g_signal_connect (GTK_WIDGET (gui->ebox), "button_press_event",
						G_CALLBACK (mouse_click), gui);
	g_signal_connect (GTK_WIDGET (gui->ebox), "scroll_event", 
						G_CALLBACK (mouse_scroll), gui);

	gui->signal_bar = gtk_progress_bar_new ();
	gtk_progress_bar_set_orientation (GTK_PROGRESS_BAR (gui->signal_bar),
						GTK_PROGRESS_LEFT_TO_RIGHT);

	gui->box = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (gui->box), BORDER / 2);
	gtk_widget_show (gui->box);

	gui->label = gtk_label_new ("");
	gtk_widget_show (gui->label);

	gtk_box_pack_start (GTK_BOX (gui->box), gui->label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (gui->box), gui->signal_bar, FALSE, FALSE,
									0);

	gtk_container_add (GTK_CONTAINER (gui->ebox), gui->box);

	return gui;
}

static void
free_presets (radio_gui *data)
{
	radio_preset *preset = data->presets;

	while (preset != NULL) {
		free(preset);
		preset = preset->next;
	}

	data->presets = NULL;
}

static void
plugin_free (radio_gui *data)
{
	if (data->on) radio_stop(data);
	free_presets (data);
	g_free (data);
}

static radio_gui *
plugin_control_new (XfcePanelPlugin *plugin)
{
	radio_gui* plugin_data = create_gui();

	plugin_data->plugin = plugin;
	
	plugin_data->on = FALSE;
	plugin_data->freq = FREQ_INIT;
	plugin_data->freqfact = 16;
	plugin_data->show_signal = TRUE;
	strcpy(plugin_data->device, "/dev/radio0");
	plugin_data->presets = NULL;
	plugin_data->scroll = CHANGE_FREQ;
	plugin_data->timeout_id = 0;
	plugin_data->tooltips = gtk_tooltips_new();
	
	update_label(plugin_data);

	gtk_container_add (GTK_CONTAINER (plugin), plugin_data->ebox);

	return plugin_data;
}

static void
radio_command_changed (GtkEditable* editable, void *pointer)
{
	radio_gui* data = (radio_gui*) pointer;
	const char* command = gtk_entry_get_text (GTK_ENTRY (editable));
	strncpy (data->command, command, MAX_COMMAND_LENGTH);
}

static void
radio_device_changed (GtkEditable* editable, void *pointer)
{
	radio_gui* data = (radio_gui*) pointer;
	const char* device = gtk_entry_get_text (GTK_ENTRY (editable));
	strncpy (data->device, device, MAX_DEVICE_NAME_LENGTH);
}

static void
radio_show_signal_changed (GtkEditable* editable, void *pointer)
{
	radio_gui* data = (radio_gui*) pointer;
	data->show_signal = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
								(editable));
	update_signal_bar (data);
}

static void
radio_scroll_type_changed (GtkEditable* button, void *pointer)
{
	radio_gui* data = (radio_gui*) pointer;
	gboolean frq = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(button));
	if (frq) {
		data->scroll = CHANGE_FREQ;
	} else {
		data->scroll = CHANGE_PRESET;
	}
}

static void
radio_plugin_dialog_response (GtkWidget *dialog, int response, radio_gui *data)
{
	gtk_widget_destroy (dialog);
	xfce_panel_plugin_unblock_menu (data->plugin);
	write_config (data->plugin, data);
}

static void
radio_plugin_create_options (XfcePanelPlugin *plugin, radio_gui *data)
{
	GtkWidget *table;
	GtkWidget *label;
	GtkWidget *hbox;
	GtkWidget *dialog;

	GSList *show_signal_group = NULL;	// signal strength:
	GtkWidget *signal_show;			//  - show
	GtkWidget *signal_hide;			//  - hide
	GtkWidget *command_entry;		// post-down command
	GtkWidget *device_entry;		// v4l-device
	GSList *scroll_group = NULL;		// scroll action:
	GtkWidget *frequency_button;		//  - change frequency
	GtkWidget *preset_button;		//  - change preset

	xfce_panel_plugin_block_menu (plugin);

	dialog = gtk_dialog_new_with_buttons (_("Properties"),
		GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
		GTK_DIALOG_DESTROY_WITH_PARENT |
		GTK_DIALOG_NO_SEPARATOR,
		GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
		NULL);

	gtk_container_set_border_width (GTK_CONTAINER (dialog), 2);

	g_signal_connect (dialog, "response",
			G_CALLBACK (radio_plugin_dialog_response), data);
	

	table = gtk_table_new(4, 2, FALSE);
	gtk_widget_show(table);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), table,
							FALSE, TRUE, 0);

	label = gtk_label_new (_("V4L device"));
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL,
								0, 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	label = gtk_label_new (_("Display signal strength"));
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL,
								0, 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox);
	gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, 1, 2, GTK_FILL,
							GTK_FILL, 0, 0);

	signal_show = gtk_radio_button_new_with_label (NULL, _("yes"));
	gtk_widget_show (signal_show);
	gtk_box_pack_start (GTK_BOX (hbox), signal_show, FALSE, FALSE, 0);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (signal_show),
							show_signal_group);
	show_signal_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON
							(signal_show));

	signal_hide = gtk_radio_button_new_with_label (show_signal_group,
								_("no"));
	gtk_widget_show (signal_hide);

	gtk_box_pack_start (GTK_BOX (hbox), signal_hide, FALSE, FALSE, 0);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (signal_show),
							data->show_signal);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (signal_hide),
							!data->show_signal);

	label = gtk_label_new (_("Execute command after shutdown"));
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4, GTK_FILL,
								0, 0, 0);
        gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

	command_entry = gtk_entry_new_with_max_length (MAX_COMMAND_LENGTH);
	gtk_entry_set_text (GTK_ENTRY (command_entry), data->command);
	gtk_widget_show (command_entry);
	gtk_table_attach (GTK_TABLE (table), command_entry, 1, 2, 3, 4,
					GTK_EXPAND | GTK_FILL, 0, 0, 0);
	
	label = gtk_label_new (_("Mouse scrolling changes"));
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3, GTK_FILL,
								0, 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox);
	gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, 2, 3, GTK_FILL,
							GTK_FILL, 0, 0);
	
	frequency_button = gtk_radio_button_new_with_label (NULL, 
							_("frequency"));
	gtk_widget_show (frequency_button);
	gtk_box_pack_start (GTK_BOX (hbox), frequency_button, FALSE, FALSE, 0);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (frequency_button),
								scroll_group);
	scroll_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON
							(frequency_button));

	preset_button = gtk_radio_button_new_with_label (NULL, _("preset"));
	gtk_widget_show (preset_button);
	gtk_box_pack_start (GTK_BOX (hbox), preset_button, FALSE, FALSE, 0);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (preset_button),
								scroll_group);
	scroll_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON
							(preset_button));

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (frequency_button),
						data->scroll == CHANGE_FREQ);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (preset_button),
						data->scroll == CHANGE_PRESET);

	device_entry = gtk_entry_new_with_max_length (MAX_DEVICE_NAME_LENGTH);
	gtk_entry_set_text (GTK_ENTRY (device_entry), data->device);
	gtk_widget_show (device_entry);
	gtk_table_attach (GTK_TABLE(table), device_entry, 1, 2, 0, 1,
					GTK_EXPAND | GTK_FILL, 0, 0, 0);

	g_signal_connect ((gpointer) command_entry, "changed",
				G_CALLBACK (radio_command_changed), data);
	g_signal_connect ((gpointer) device_entry, "changed",
				G_CALLBACK (radio_device_changed), data);
	g_signal_connect (G_OBJECT (signal_show), "toggled",
				G_CALLBACK(radio_show_signal_changed), data);
	g_signal_connect (G_OBJECT (frequency_button), "toggled",
				G_CALLBACK (radio_scroll_type_changed), data);

	gtk_widget_show (dialog);
}

static void
radio_plugin_set_size (XfcePanelPlugin *plugin, int size, radio_gui *data)
{
	gtk_widget_set_size_request (data->signal_bar, size - BORDER, BORDER);
}

static void
write_config (XfcePanelPlugin *plugin, radio_gui *data)
{
	char *file;
	XfceRc *rc;

	if (!(file = xfce_panel_plugin_save_location (plugin, TRUE)))
		return;

	rc = xfce_rc_simple_open (file, FALSE);
	g_free (file);

	if (!rc)
		return;

	xfce_rc_set_group	(rc, "radio plugin");

	xfce_rc_write_entry	(rc, "dev", data->device);
	xfce_rc_write_entry	(rc, "cmd", data->command);
	xfce_rc_write_int_entry (rc, "frq", data->freq);
	xfce_rc_write_int_entry (rc, "scroll", data->scroll);
	xfce_rc_write_bool_entry(rc, "show_signal", data->show_signal);

	xfce_rc_set_group	(rc, "presets");

	radio_preset *preset = data->presets, *prev;

	char buf[6];

	while (preset != NULL) {
		sprintf(buf, "%d", preset->freq);
		xfce_rc_write_entry (rc, buf, preset->name);
		preset = preset->next;
	}

	xfce_rc_close (rc);
}

static void
read_config (XfcePanelPlugin *plugin, radio_gui *data)
{
	const char *value;
	char *file;
	XfceRc *rc;

	if (!(file = xfce_panel_plugin_lookup_rc_file (plugin)))
		return;

	rc = xfce_rc_simple_open (file, TRUE);
	g_free (file);

	if (!rc)
		return;

	xfce_rc_set_group (rc, "radio plugin");

	data->freq = xfce_rc_read_int_entry (rc, "frq", FREQ_INIT);
	data->scroll = xfce_rc_read_int_entry (rc, "scroll", CHANGE_FREQ);
	data->show_signal = xfce_rc_read_bool_entry (rc, "show_signal", TRUE);

	if ((value = xfce_rc_read_entry (rc, "dev", NULL)) && *value) {
		strncpy (data->device, value, MAX_DEVICE_NAME_LENGTH);
	}
	if ((value = xfce_rc_read_entry (rc, "cmd", NULL)) && *value) {
		strncpy (data->command, value, MAX_COMMAND_LENGTH);
	}

	free_presets (data);

	gchar** entries = xfce_rc_get_entries (rc, "presets");
	if (entries != NULL) {
		gchar** entry;
		entry = entries;

		xfce_rc_set_group (rc, "presets");

		while (*entry) {
			if ((value = xfce_rc_read_entry (rc, *entry, NULL)) 
								&& *value) {
				radio_preset *preset = 
						malloc (sizeof (radio_preset));
				preset->next = NULL;
				strncpy(preset->name, value,
							MAX_PRESET_NAME_LENGTH);
				preset->freq = atoi (*entry);
				append_to_presets (preset, data);
			}
			*entry++;
		}
	}

	// TODO: free value (?)
	g_strfreev (entries);
}

static void
radio_plugin_construct (XfcePanelPlugin *plugin)
{
	radio_gui *data = plugin_control_new (plugin);
	read_config(plugin, data);
	
	xfce_panel_plugin_menu_show_configure (plugin);

	g_signal_connect (plugin, "configure-plugin",
			G_CALLBACK (radio_plugin_create_options), data);
	g_signal_connect (plugin, "size-changed", G_CALLBACK
						(radio_plugin_set_size), data);
	g_signal_connect (plugin, "free-data", G_CALLBACK (plugin_free), data);
	g_signal_connect (plugin, "save", G_CALLBACK (write_config), data);
}

XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL (radio_plugin_construct);
