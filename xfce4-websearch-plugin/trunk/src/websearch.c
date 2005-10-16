/*
 * $Id$
 * Copyright (c) 2005 by Piotr Wolny <gildur@gmail.com>
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

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4panel/xfce-panel-plugin.h>

#define WEBSEARCH_SETTINGS "WebSearch"

#define DISPLAY_CHARS 10

typedef struct
{
    gchar* name;
    gchar* uri;
} SearchEngine;

typedef struct
{
    gint count;
    gint selected;
    SearchEngine** engines;
} SearchEngines;

typedef struct
{
    GtkWidget* eventbox;
    GtkWidget* entry;

    gchar* browser_command;
    SearchEngines* engines;
} WebSearchPlugin;

typedef struct
{
    XfcePanelPlugin* plugin;
    WebSearchPlugin* websearch;
    GtkWidget* browser_entry;
    GtkWidget* engine_combo;
} OptionsInput;

static gboolean entry_buttonpress_callback(GtkWidget* entry,
        GdkEventButton* event, gpointer data)
{
    static Atom atom = 0;
    GtkWidget* toplevel = gtk_widget_get_toplevel(entry);

    if (event->button != 3 && toplevel && toplevel->window)
    {
        XClientMessageEvent xev;

        if (G_UNLIKELY(!atom))
        {
            atom = XInternAtom(GDK_DISPLAY(), "_NET_ACTIVE_WINDOW", FALSE);
        }

        xev.type = ClientMessage;
        xev.window = GDK_WINDOW_XID(toplevel->window);
        xev.message_type = atom;
        xev.format = 32;
        xev.data.l[0] = 0;
        xev.data.l[1] = 0;
        xev.data.l[2] = 0;
        xev.data.l[3] = 0;
        xev.data.l[4] = 0;

        XSendEvent(GDK_DISPLAY(), GDK_ROOT_WINDOW(), FALSE,
                StructureNotifyMask, (XEvent*) &xev);

        gtk_widget_grab_focus(entry);
    }

    return FALSE;
}

static gboolean entry_keypress_callback(GtkWidget* entry, GdkEventKey* event,
        WebSearchPlugin* websearch)
{
    SearchEngines* engines = websearch->engines;
    gchar* execute = NULL;

    switch (event->keyval)
    {
        case GDK_Return:
            execute = g_strconcat(
                    websearch->browser_command,
                    " ",
                    engines->engines[engines->selected]->uri,
                    gtk_entry_get_text(GTK_ENTRY(entry)),
                    NULL);
            exec_command(execute);
            g_free(execute);

            gtk_entry_set_text(GTK_ENTRY(entry), "");
            return TRUE;
        default:
            return FALSE;
    }
}

static SearchEngines* search_engines_new()
{
    SearchEngines* engines = g_new(SearchEngines, 1);
    
    engines->count = 5;
    engines->selected = 2;
    engines->engines = g_new(SearchEngine*, engines->count);
    
    engines->engines[0] = g_new(SearchEngine, 1);
    engines->engines[0]->name = g_strdup("Altavista");
    engines->engines[0]->uri = 
            g_strdup("http://www.altavista.com/web/results?q=");
    
    engines->engines[1] = g_new(SearchEngine, 1);
    engines->engines[1]->name = g_strdup("Creative Commons");
    engines->engines[1]->uri = 
            g_strdup("http://search.creativecommons.org/?q=");

    engines->engines[2] = g_new(SearchEngine, 1);
    engines->engines[2]->name = g_strdup("Google");
    engines->engines[2]->uri = 
            g_strdup("http://www.google.com/search?q=");

    engines->engines[3] = g_new(SearchEngine, 1);
    engines->engines[3]->name = g_strdup("Yahoo");
    engines->engines[3]->uri = 
            g_strdup("http://search.yahoo.com/search?p=");

    engines->engines[4] = g_new(SearchEngine, 1);
    engines->engines[4]->name = g_strdup("Wikipedia");
    engines->engines[4]->uri = 
            g_strdup("http://en.wikipedia.org/wiki/");

    return engines;
}

static WebSearchPlugin* websearch_new()
{
    WebSearchPlugin* websearch = g_new(WebSearchPlugin, 1);

    websearch->eventbox = gtk_event_box_new();
    gtk_widget_show(websearch->eventbox);

    websearch->entry = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(websearch->entry), DISPLAY_CHARS);
    gtk_widget_show(websearch->entry);

    gtk_container_add(GTK_CONTAINER(websearch->eventbox), websearch->entry);
    
    websearch->browser_command = g_strdup("firefox");
    websearch->engines = search_engines_new();
    
    g_signal_connect(websearch->entry, "key-press-event", 
            G_CALLBACK(entry_keypress_callback), websearch);
    g_signal_connect(websearch->entry, "button-press-event",
            G_CALLBACK(entry_buttonpress_callback), NULL);
    
    return websearch;
}
 
static void websearch_free(XfcePanelPlugin* plugin, WebSearchPlugin* websearch)
{
    int i = 0;

    for (i = 0; i < websearch->engines->count; i++)
    {
        g_free(websearch->engines->engines[i]->name);
        g_free(websearch->engines->engines[i]->uri);
        g_free(websearch->engines->engines[i]);
    }

    g_free(websearch->engines->engines);
    g_free(websearch->engines);
    g_free(websearch->browser_command);
    g_free(websearch);
}

static void websearch_read_config(XfcePanelPlugin* plugin,
        WebSearchPlugin* websearch)
{
    char* file = NULL;
    const gchar* value = NULL;
    XfceRc* rc = NULL;

    if ((file = xfce_panel_plugin_lookup_rc_file(plugin)))
    {
        rc = xfce_rc_simple_open(file, TRUE);
        g_free(file);
        
        if (rc)
        {
            if (xfce_rc_has_group(rc, WEBSEARCH_SETTINGS))
            {
                xfce_rc_set_group(rc, WEBSEARCH_SETTINGS);

                if ((value = xfce_rc_read_entry(rc, "browserCommand", NULL)))
                {
                    g_free(websearch->browser_command);
                    websearch->browser_command = g_strdup(value);
                }
                if ((value = xfce_rc_read_entry(rc, "selectedEngine", NULL)))
                {
                    websearch->engines->selected = strtol(value, NULL, 10);
                }
            }
            xfce_rc_close(rc);
        }
    }                                
}

static void websearch_write_config(XfcePanelPlugin* plugin,
        WebSearchPlugin* websearch)
{
    char* file = NULL;
    gchar* value = NULL;
    XfceRc* rc = NULL;

    if ((file = xfce_panel_plugin_save_location(plugin, TRUE)))
    {
        rc = xfce_rc_simple_open(file, FALSE);
        g_free(file);

        if (rc)
        {
            xfce_rc_set_group(rc, WEBSEARCH_SETTINGS);
            xfce_rc_write_entry(rc, "browserCommand", 
                    websearch->browser_command);
            value = g_strdup_printf("%d", websearch->engines->selected);
            xfce_rc_write_entry(rc, "selectedEngine", value);
            g_free(value);
            xfce_rc_close(rc);
        }
    }
}

static OptionsInput* options_input_new(XfcePanelPlugin* plugin,
        WebSearchPlugin* websearch)
{
    SearchEngines* engines = websearch->engines;
    OptionsInput* input = g_new(OptionsInput, 1);
    int i = 0;
    
    input->plugin = plugin;
    input->websearch = websearch;
    input->browser_entry = gtk_entry_new();
    input->engine_combo = gtk_combo_box_new_text();
    
    gtk_entry_set_text(GTK_ENTRY(input->browser_entry),
            websearch->browser_command);
    
    for(i = 0; i < engines->count; i++)
    {
        gtk_combo_box_append_text(GTK_COMBO_BOX(input->engine_combo),
                engines->engines[i]->name);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(input->engine_combo),
            engines->selected);

    return input;
}

static void apply_options_callback(GtkDialog* dialog, gint response, 
        OptionsInput* input)
{
    if (response == GTK_RESPONSE_OK)
    {
        WebSearchPlugin* websearch = input->websearch;

        g_free(websearch->browser_command);
        websearch->browser_command = 
                g_strdup(gtk_entry_get_text(GTK_ENTRY(input->browser_entry)));

        websearch->engines->selected = 
                gtk_combo_box_get_active(GTK_COMBO_BOX(input->engine_combo));
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
    xfce_panel_plugin_unblock_menu(input->plugin);
    g_free(input);
}

static void websearch_create_options(XfcePanelPlugin* plugin,
        WebSearchPlugin* websearch)
{
    GtkWidget* dialog = NULL;
    GtkWidget* layout = gtk_table_new(2, 2, FALSE);
    GtkWidget* label = NULL;
    OptionsInput* input = options_input_new(plugin, websearch);

    xfce_panel_plugin_block_menu(plugin);
    dialog = gtk_dialog_new_with_buttons(_("Properties"),
            GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(plugin))),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_STOCK_OK, GTK_RESPONSE_OK,
            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);

    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), layout, 
            TRUE, TRUE, 10);

    label = gtk_label_new(_("Browser command: "));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_table_attach(GTK_TABLE(layout), label,
            0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
    gtk_table_attach(GTK_TABLE(layout), input->browser_entry,
            1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
    
    label = gtk_label_new(_("Search engine: "));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_table_attach(GTK_TABLE(layout), label,
            0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
    gtk_table_attach(GTK_TABLE(layout), input->engine_combo,
            1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
    
    g_signal_connect(dialog, "response",
            G_CALLBACK(apply_options_callback), input);
    
    gtk_widget_show_all(dialog);
}

static void websearch_about(XfcePanelPlugin* plugin, WebSearchPlugin* websearch)
{
    GtkWidget* dialog = gtk_about_dialog_new();
    const gchar* authors[] = { "Piotr Wolny <gildur@gmail.com>", NULL };
    gtk_about_dialog_set_name(GTK_ABOUT_DIALOG(dialog), _("WebSearch"));
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), VERSION);
    gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog), 
            "Copyright (c) 2005 by Piotr Wolny\nLicensed under GNU GPL");
    gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog), 
            "http://xfce-goodies.berlios.de");
    gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dialog), authors);
    gtk_widget_show_all(dialog);
}

static void websearch_construct(XfcePanelPlugin* plugin)
{
    WebSearchPlugin* websearch = NULL;
    xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
   
    websearch = websearch_new();
    websearch_read_config(plugin, websearch);

    gtk_container_add(GTK_CONTAINER(plugin), websearch->eventbox);
    xfce_panel_plugin_add_action_widget(plugin, websearch->eventbox);
    xfce_panel_plugin_add_action_widget(plugin, websearch->entry);

    g_signal_connect(plugin, "free-data", G_CALLBACK(websearch_free), 
            websearch);
    
    g_signal_connect(plugin, "save", G_CALLBACK(websearch_write_config),
            websearch);

    xfce_panel_plugin_menu_show_configure(plugin);
    g_signal_connect(plugin, "configure-plugin",
            G_CALLBACK(websearch_create_options), websearch);
    
    xfce_panel_plugin_menu_show_about(plugin);
    g_signal_connect(plugin, "about", G_CALLBACK(websearch_about), websearch);
}

XFCE_PANEL_PLUGIN_REGISTER_INTERNAL(websearch_construct);
