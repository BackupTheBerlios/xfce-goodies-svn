/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright (c) 2006 Nick Schermer <nick@xfce.org>
 *  Copyright (c) 2006 Benedikt Meurer <benny@xfce.org>
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

#define BORDER 5

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <gtk/gtk.h>

#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4panel/xfce-panel-plugin.h>

#include "battery.h"
#include "battery-dialogs.h"

typedef struct
{
    BatteryPlugin *battery;

    GtkWidget *panel_icon, *panel_progressbar, *panel_percentage, *panel_time;

    GtkWidget *tooltip_time;
    GtkWidget *main_battery;

    GtkWidget *spin_critical,   *spin_low;
    GtkWidget *combo_critical,  *combo_low,  *combo_full;
    GtkWidget *entry_critical,  *entry_low,  *entry_full;
    GtkWidget *button_critical, *button_low, *button_full;
}
BatteryOptions;

static void
check_button_toggled (GtkWidget      *button,
                      BatteryOptions *options)
{
    if (button == options->panel_icon)
        options->battery->show_icon = 
            gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
    
    else if (button == options->panel_progressbar)
        options->battery->show_progressbar = 
            gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

    else if (button == options->panel_percentage)
        options->battery->show_percentage = 
            gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

    else if (button == options->panel_time)
        options->battery->show_time = 
            gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

    else if (button == options->tooltip_time)
        options->battery->tip_time = 
            gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
    
    battery_widgets (options->battery);
    battery_update_plugin (options->battery);
}
/*
static gboolean
return_valid_command (gchar *command)
{
    // Script from Thunar UCA Editor *
    gchar *filename = g_strdup (command);
    gchar *s;

    if (G_LIKELY (filename != NULL))
    {
        // use only the first argument *
        s = strchr (filename, ' ');
        if (G_UNLIKELY (s != NULL))
            *s = '\0';

        // check if we have a file name *
        if (G_LIKELY (*filename != '\0'))
        {
            // check if the filename is not an absolute path *
            if (G_LIKELY (!g_path_is_absolute (filename)))
            {
                // try to lookup the filename in $PATH *
                s = g_find_program_in_path (filename);
                if (G_LIKELY (s != NULL))
                {
                    // use the absolute path instead *
                    g_free (filename);
                    filename = s;
                }
            }

            // check if we have an absolute path now *
            if (G_LIKELY (g_path_is_absolute (filename)))
            {
                DBG ("Valid command: %s", filename);
                g_free (filename);
                return TRUE;
            }
        }
    }

    DBG ("Not A valid command");
    g_free (filename);
    return FALSE;
}
*/
static gboolean
command_focus_out (GtkWidget      *entry,
                   GdkEventFocus  *event,
                   BatteryOptions *options)
{
    gchar *command = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);

    //if (!command || /*!return_valid_command (command)*/)
    if (!command)
    {
        DBG ("WARNING, NOT A VALID COMMAND");
    }

    if (entry == options->entry_critical)
        options->battery->command_critical = g_strdup (command);
    
    else if (entry == options->entry_low)
        options->battery->command_low = g_strdup (command);
    
    else if (entry == options->entry_full)
        options->battery->command_charged = g_strdup (command);
    
    g_free (command);
    
    return FALSE;
}

static void
combobox_changed_sensative (gint       active,
                            GtkWidget *entry,
                            GtkWidget *button)
{
    if (active >= 2)
    {
        gtk_widget_set_sensitive (entry, TRUE);
        gtk_widget_set_sensitive (button, TRUE);
    }
    else
    {
        gtk_widget_set_sensitive (entry, FALSE);
        gtk_widget_set_sensitive (button, FALSE);
    }
}

static void
combobox_changed (GtkComboBox    *combobox,
                  BatteryOptions *options)
{
    guint active = gtk_combo_box_get_active (combobox);

    if (GTK_WIDGET(combobox) == options->combo_critical)
    {
        combobox_changed_sensative (active, options->entry_critical, options->button_critical);
        options->battery->action_critical = active;
    }
    else if (GTK_WIDGET(combobox) == options->combo_low)
    {
        combobox_changed_sensative (active, options->entry_low, options->button_low);
        options->battery->action_low = active;
    }
    else if (GTK_WIDGET(combobox) == options->combo_full)
    {
        combobox_changed_sensative (active, options->entry_full, options->button_full);
        options->battery->action_charged = active;
    }
    else if (GTK_WIDGET (combobox) == options->main_battery )
    {
        options->battery->show_battery = active;
    
        battery_widgets (options->battery);
        battery_update_plugin (options->battery);
    }
}

static void
spin_button_changed (GtkWidget      *button,
                     BatteryOptions *options)
{
    guint value = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(button)); 

    if (button == options->spin_critical)
        options->battery->perc_critical = value;
    else if (button == options->spin_low)
        options->battery->perc_low = value;
}

static void
battery_configure_response (GtkWidget      *dialog, 
                            int             response, 
                            BatteryOptions *options)
{
    g_object_set_data (G_OBJECT (options->battery->plugin), "configure", NULL);

    xfce_panel_plugin_unblock_menu (options->battery->plugin);

    /* Destroy all widgets */
    gtk_widget_destroy (dialog);

    /* Save new settings */
    battery_save (options->battery->plugin, options->battery);
    
    g_free (options);
}

static void
battery_command_clicked (GtkWidget      *button,
                         GdkEventButton *ev,
                         GtkWidget      *entry)
{
    GtkFileFilter *filter;
    GtkWidget     *chooser;
    gchar         *filename;
    gchar         *s;

    chooser = gtk_file_chooser_dialog_new (_("Select an Application"),
                                           NULL,
                                           GTK_FILE_CHOOSER_ACTION_OPEN,
                                           GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                           GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                           NULL);
    gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (chooser), TRUE);

    // add file chooser filters *
    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, _("All Files"));
    gtk_file_filter_add_pattern (filter, "*");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), filter);

    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, _("Executable Files"));
    gtk_file_filter_add_mime_type (filter, "application/x-csh");
    gtk_file_filter_add_mime_type (filter, "application/x-executable");
    gtk_file_filter_add_mime_type (filter, "application/x-perl");
    gtk_file_filter_add_mime_type (filter, "application/x-python");
    gtk_file_filter_add_mime_type (filter, "application/x-ruby");
    gtk_file_filter_add_mime_type (filter, "application/x-shellscript");
    gtk_file_filter_add_pattern (filter, "*.pl");
    gtk_file_filter_add_pattern (filter, "*.py");
    gtk_file_filter_add_pattern (filter, "*.rb");
    gtk_file_filter_add_pattern (filter, "*.sh");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), filter);
    gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (chooser), filter);

    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, _("Perl Scripts"));
    gtk_file_filter_add_mime_type (filter, "application/x-perl");
    gtk_file_filter_add_pattern (filter, "*.pl");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), filter);

    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, _("Python Scripts"));
    gtk_file_filter_add_mime_type (filter, "application/x-python");
    gtk_file_filter_add_pattern (filter, "*.py");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), filter);

    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, _("Ruby Scripts"));
    gtk_file_filter_add_mime_type (filter, "application/x-ruby");
    gtk_file_filter_add_pattern (filter, "*.rb");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), filter);

    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, _("Shell Scripts"));
    gtk_file_filter_add_mime_type (filter, "application/x-csh");
    gtk_file_filter_add_mime_type (filter, "application/x-shellscript");
    gtk_file_filter_add_pattern (filter, "*.sh");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), filter);

    // use the bindir as default folder *
    // gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser), BINDIR); *

    // setup the currently selected file *
    filename = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
    if (G_LIKELY (filename != NULL))
    {
        // use only the first argument *
        s = strchr (filename, ' ');
        if (G_UNLIKELY (s != NULL))
            *s = '\0';

        // check if we have a file name *
        if (G_LIKELY (*filename != '\0'))
        {
            // check if the filename is not an absolute path *
            if (G_LIKELY (!g_path_is_absolute (filename)))
            {
                // try to lookup the filename in $PATH *
                s = g_find_program_in_path (filename);
                if (G_LIKELY (s != NULL))
                {
                    // use the absolute path instead *
                    g_free (filename);
                    filename = s;
                }
            }

            // check if we have an absolute path now *
            if (G_LIKELY (g_path_is_absolute (filename)))
                gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (chooser), filename);
        }

        // release the filename *
        g_free (filename);
    }

    // run the chooser dialog *
    if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
    {
        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
        s = g_strconcat (filename, " %f", NULL);
        gtk_entry_set_text (GTK_ENTRY (entry), filename);
        g_free (filename);
        g_free (s);
    }

    gtk_widget_destroy (chooser);
}

void
battery_configure (XfcePanelPlugin *plugin,
		   BatteryPlugin   *battery)
{
    GtkWidget      *dialog, *dialog_vbox, *notebook, *notebook_vbox;
    GtkWidget      *frame, *button, *icon, *label, *alignment, *hbox, *vbox;
    GtkWidget      *combobox, *entry;

#ifndef USE_NEW_DIALOG
    GtkWidget      *header;
#endif

    GtkSizeGroup   *sg_buttons, *sg_labels;
    guint           i;
    gchar          *name;
    BatteryOptions *options;

    DBG ("Show Properties dialog");

    sg_buttons = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
    sg_labels  = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

    options = g_new0 (BatteryOptions, 1);

    options->battery = battery;

    xfce_panel_plugin_block_menu (battery->plugin);

#ifdef USE_NEW_DIALOG
    dialog = xfce_titled_dialog_new_with_buttons (_("Battery Monitor"),
                                                  NULL,
                                                  GTK_DIALOG_NO_SEPARATOR,
                                                  GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                                                  NULL);
    xfce_titled_dialog_set_subtitle (XFCE_TITLED_DIALOG (dialog),
                                     _("Configure the battery monitor plugin"));
#else
    dialog = gtk_dialog_new_with_buttons (_("Configure Battery Monitor"), 
                                          GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
                                          GTK_DIALOG_DESTROY_WITH_PARENT |
                                          GTK_DIALOG_NO_SEPARATOR,
                                          GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                                          NULL);
#endif

    gtk_window_set_position   (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
    gtk_window_set_icon_name  (GTK_WINDOW (dialog), "gtk-properties");
    gtk_window_set_keep_above (GTK_WINDOW (dialog), TRUE);
    gtk_window_stick          (GTK_WINDOW (dialog));
    
    g_object_set_data (G_OBJECT (battery->plugin), "configure", dialog);
    
    dialog_vbox = GTK_DIALOG (dialog)->vbox;

#ifndef USE_NEW_DIALOG
    header = xfce_create_header (NULL, _("Battery Monitor"));
    gtk_widget_set_size_request (GTK_BIN (header)->child, -1, 32);
    gtk_container_set_border_width (GTK_CONTAINER (header), BORDER);
    gtk_box_pack_start (GTK_BOX (dialog_vbox), header, FALSE, TRUE, 0);
#endif

    notebook = gtk_notebook_new ();
    gtk_box_pack_start (GTK_BOX (dialog_vbox), notebook, FALSE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (notebook), BORDER);
    
    /* Appearance tab */
    notebook_vbox = gtk_vbox_new (FALSE, 2);
    gtk_container_add (GTK_CONTAINER (notebook), notebook_vbox);
    
    /* Panel Appearance frame */ 
    frame = gtk_frame_new (NULL);
    gtk_box_pack_start (GTK_BOX (notebook_vbox), frame, FALSE, TRUE, 0);
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);

    alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_container_add (GTK_CONTAINER (frame), alignment);
    gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, BORDER*3, 0);

    vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_container_add (GTK_CONTAINER (alignment), vbox);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), BORDER);
    
    /* Options for users with more than one battery */
    if (G_UNLIKELY (battery->batteries->len > 1))
    {
        hbox = gtk_hbox_new (FALSE, BORDER);
        gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    
        label = gtk_label_new (_("Display Battery:"));
        gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
        gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    
        combobox = options->main_battery = gtk_combo_box_new_text ();
        gtk_box_pack_start (GTK_BOX (hbox), combobox, FALSE, TRUE, 0);
    
        for (i = 0; i < battery->batteries->len; ++i)
        {
            /* Idea: add some kind of tooltip with the udi */
            
            name = g_strdup_printf (_("Battery %d"), i+1);
            
            gtk_combo_box_append_text (GTK_COMBO_BOX (combobox), name);
            
            g_free (name);
        }
        
        gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), battery->show_battery);
    
        g_signal_connect(G_OBJECT(combobox), "changed",
            G_CALLBACK(combobox_changed), options);
    }

    button = options->panel_icon = gtk_check_button_new_with_mnemonic (_("Show battery status icon"));
    gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), battery->show_icon);
    
    g_signal_connect (button, "toggled",
        G_CALLBACK (check_button_toggled), options);
        
    button = options->panel_progressbar = gtk_check_button_new_with_mnemonic (_("Show battery progressbar"));
    gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), battery->show_progressbar);
    
    g_signal_connect (button, "toggled",
        G_CALLBACK (check_button_toggled), options);

    button = options->panel_percentage = gtk_check_button_new_with_mnemonic (_("Show percentage"));
    gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), battery->show_percentage);
    
    g_signal_connect (button, "toggled",
        G_CALLBACK (check_button_toggled), options);

    button = options->panel_time = gtk_check_button_new_with_mnemonic (_("Show time remaining"));
    gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), battery->show_time);
    
    g_signal_connect (button, "toggled",
        G_CALLBACK (check_button_toggled), options);

    label = gtk_label_new (_("<b>Panel</b>"));
    gtk_frame_set_label_widget (GTK_FRAME (frame), label);
    gtk_label_set_use_markup (GTK_LABEL (label), TRUE);

    /* Tooltip appearance frame */
    frame = gtk_frame_new (NULL);
    gtk_box_pack_start (GTK_BOX (notebook_vbox), frame, TRUE, TRUE, 0);
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);

    alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_container_add (GTK_CONTAINER (frame), alignment);
    gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, BORDER*3, 0);

    vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_container_add (GTK_CONTAINER (alignment), vbox);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), BORDER);

    button = options->tooltip_time = gtk_check_button_new_with_mnemonic (_("Show time remaining"));
    gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), battery->tip_time);
    
    g_signal_connect (button, "toggled",
        G_CALLBACK (check_button_toggled), options);

    label = gtk_label_new (_("<b>Tooltip</b>"));
    gtk_frame_set_label_widget (GTK_FRAME (frame), label);
    gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
    
    /* Appearance tab label */
    label = gtk_label_new (_("Appearance"));
    gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), 0), label);
    
    /* Actions tab */
    notebook_vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_container_add (GTK_CONTAINER (notebook), notebook_vbox);
    gtk_container_set_border_width (GTK_CONTAINER (notebook_vbox), BORDER);
    
    /* Critical battery action frame */
    frame = gtk_frame_new (NULL);
    gtk_box_pack_start (GTK_BOX (notebook_vbox), frame, TRUE, TRUE, 0);
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);

    alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_container_add (GTK_CONTAINER (frame), alignment);
    gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, BORDER*3, 0);

    vbox = gtk_vbox_new (FALSE, 2);
    gtk_container_add (GTK_CONTAINER (alignment), vbox);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), BORDER);

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    label = gtk_label_new (_("Percentage:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    
    gtk_size_group_add_widget (sg_labels, label);

    button = options->spin_critical = gtk_spin_button_new_with_range(0, 99, 1);
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(button), battery->perc_critical);
    
    g_signal_connect (G_OBJECT (button), "value_changed",
            G_CALLBACK (spin_button_changed), options);

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    label = gtk_label_new (_("Action:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    
    gtk_size_group_add_widget (sg_labels, label);

    combobox = options->combo_critical = gtk_combo_box_new_text ();
    gtk_box_pack_start (GTK_BOX (hbox), combobox, TRUE, TRUE, 0);
    gtk_combo_box_append_text (GTK_COMBO_BOX (combobox), _("Do nothing"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (combobox), _("Display warning message"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (combobox), _("Run Command"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (combobox), _("Run command in terminal"));
    
    g_signal_connect(G_OBJECT(combobox), "changed",
        G_CALLBACK(combobox_changed), options);

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    label = gtk_label_new (_("Command:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    
    gtk_size_group_add_widget (sg_labels, label);

    entry = options->entry_critical = gtk_entry_new ();
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    
    if (battery->command_critical)
        gtk_entry_set_text (GTK_ENTRY (entry), battery->command_critical);
    
    g_signal_connect (G_OBJECT(entry), "focus-out-event",
        G_CALLBACK (command_focus_out), options);

    button = options->button_critical = gtk_button_new ();
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
    
    g_signal_connect(button, "button_press_event",
            G_CALLBACK(battery_command_clicked), entry);

    icon = gtk_image_new_from_stock ("gtk-open", GTK_ICON_SIZE_MENU);
    gtk_container_add (GTK_CONTAINER (button), icon);
    
    gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), battery->action_critical);

    label = gtk_label_new (_("<b>Critical Battery</b>"));
    gtk_frame_set_label_widget (GTK_FRAME (frame), label);
    gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
    
    /* Low battery action frame */
    frame = gtk_frame_new (NULL);
    gtk_box_pack_start (GTK_BOX (notebook_vbox), frame, TRUE, TRUE, 0);
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);

    alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_container_add (GTK_CONTAINER (frame), alignment);
    gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, BORDER*3, 0);

    vbox = gtk_vbox_new (FALSE, 2);
    gtk_container_add (GTK_CONTAINER (alignment), vbox);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), BORDER);

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    label = gtk_label_new (_("Percentage:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    
    gtk_size_group_add_widget (sg_labels, label);

    button = options->spin_low = gtk_spin_button_new_with_range(0, 99, 1);
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(button), battery->perc_low);
    
    g_signal_connect (G_OBJECT (button), "value_changed",
            G_CALLBACK (spin_button_changed), options);

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    label = gtk_label_new (_("Action:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    
    gtk_size_group_add_widget (sg_labels, label);

    combobox = options->combo_low = gtk_combo_box_new_text ();
    gtk_box_pack_start (GTK_BOX (hbox), combobox, TRUE, TRUE, 0);
    gtk_combo_box_append_text (GTK_COMBO_BOX (combobox), _("Do nothing"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (combobox), _("Display warning message"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (combobox), _("Run Command"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (combobox), _("Run command in terminal"));
    
    g_signal_connect(G_OBJECT(combobox), "changed",
        G_CALLBACK(combobox_changed), options);

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    label = gtk_label_new (_("Command:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    
    gtk_size_group_add_widget (sg_labels, label);

    entry = options->entry_low = gtk_entry_new ();
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    
    if (battery->command_low)
        gtk_entry_set_text (GTK_ENTRY (entry), battery->command_low);
    
    g_signal_connect (G_OBJECT(entry), "focus-out-event",
        G_CALLBACK (command_focus_out), options);

    button = options->button_low = gtk_button_new ();
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
    
    g_signal_connect(button, "button_press_event",
            G_CALLBACK(battery_command_clicked), entry);

    icon = gtk_image_new_from_stock ("gtk-open", GTK_ICON_SIZE_MENU);
    gtk_container_add (GTK_CONTAINER (button), icon);

    gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), battery->action_low);

    label = gtk_label_new (_("<b>Low Battery</b>"));
    gtk_frame_set_label_widget (GTK_FRAME (frame), label);
    gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
    
    /* Charged battery action frame */
    frame = gtk_frame_new (NULL);
    gtk_box_pack_start (GTK_BOX (notebook_vbox), frame, TRUE, TRUE, 0);
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);

    alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_container_add (GTK_CONTAINER (frame), alignment);
    gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, BORDER*3, 0);

    vbox = gtk_vbox_new (FALSE, 2);
    gtk_container_add (GTK_CONTAINER (alignment), vbox);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), BORDER);

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    label = gtk_label_new (_("Action:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    
    gtk_size_group_add_widget (sg_labels, label);

    combobox = options->combo_full = gtk_combo_box_new_text ();
    gtk_box_pack_start (GTK_BOX (hbox), combobox, TRUE, TRUE, 0);
    gtk_combo_box_append_text (GTK_COMBO_BOX (combobox), _("Do nothing"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (combobox), _("Display warning message"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (combobox), _("Run Command"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (combobox), _("Run command in terminal"));
    
    g_signal_connect(G_OBJECT(combobox), "changed",
        G_CALLBACK(combobox_changed), options);

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    label = gtk_label_new (_("Command:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    
    gtk_size_group_add_widget (sg_labels, label);

    entry = options->entry_full = gtk_entry_new ();
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    
    if (battery->command_charged)
        gtk_entry_set_text (GTK_ENTRY (entry), battery->command_charged);
    
    g_signal_connect (G_OBJECT(entry), "focus-out-event",
        G_CALLBACK (command_focus_out), options);

    button = options->button_full = gtk_button_new ();
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
    
    g_signal_connect(button, "button_press_event",
            G_CALLBACK(battery_command_clicked), entry);

    icon = gtk_image_new_from_stock ("gtk-open", GTK_ICON_SIZE_MENU);
    gtk_container_add (GTK_CONTAINER (button), icon);

     gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), battery->action_charged);

    label = gtk_label_new (_("<b>Battery Fully Charged</b>"));
    gtk_frame_set_label_widget (GTK_FRAME (frame), label);
    gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
    
    /* Actions tab label */
    label = gtk_label_new (_("Actions"));
    gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), 1), label);
    
    g_signal_connect(dialog, "response",
        G_CALLBACK(battery_configure_response), options);
        
    gtk_widget_show_all (dialog);

}
