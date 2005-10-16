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

#include <panel/plugins.h>

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
    WebSearchPlugin* websearch;
    GtkWidget* browser_entry;
    GtkWidget* engine_combo;
} OptionsInput;

static gboolean entry_buttonpress_callback(GtkWidget* entry,
        GdkEventButton* event, gpointer data)
{
    static Atom atom = 0;
    GtkWidget* toplevel = gtk_widget_get_toplevel(entry);

    if(event->button != 3 && toplevel && toplevel->window)
    {
        XClientMessageEvent xev;

        if(G_UNLIKELY(!atom))
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
        gpointer user_data)
{
    WebSearchPlugin* websearch = (WebSearchPlugin*) user_data;
    SearchEngines* engines = websearch->engines;
    gchar* execute = NULL;

    switch(event->keyval)
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
 
static gboolean websearch_create_control(Control* control)
{
    WebSearchPlugin *websearch = websearch_new();

    gtk_container_add(GTK_CONTAINER(control->base), websearch->eventbox);

    control->data = (gpointer) websearch;
    control->with_popup = FALSE;

    gtk_widget_set_size_request(control->base, -1, -1);

    return TRUE;
}

static void websearch_free(Control* control)
{
    WebSearchPlugin* websearch = (WebSearchPlugin*) control->data;
    int i = 0;

    for(i = 0; i < websearch->engines->count; i++)
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

static void websearch_attach_callback(Control* control, const char* signal,
        GCallback callback, gpointer data)
{
    WebSearchPlugin* websearch = (WebSearchPlugin*) control->data;
    
    g_signal_connect(websearch->eventbox, signal, callback, data);
    g_signal_connect(websearch->entry, signal, callback, data);
}

static void websearch_read_config(Control* control, xmlNodePtr node)
{
    WebSearchPlugin* websearch = (WebSearchPlugin*) control->data;
    xmlChar* value = NULL;

    if((node = node->children) == NULL) return;   
    if(xmlStrEqual(node->name, (const xmlChar*) "browserCommand"))
    {
        value = xmlNodeGetContent(node);
        g_free(websearch->browser_command);
        websearch->browser_command = g_strdup((char*) value);
        if((node = node->next) == NULL) return;
    }
    
    if(xmlStrEqual(node->name, (const xmlChar*) "selectedEngine"))
    {
        value = xmlNodeGetContent(node);
        websearch->engines->selected = strtol((char*) value, NULL, 10);
    }
}

static void websearch_write_config(Control* control, xmlNodePtr parent)
{
    WebSearchPlugin* websearch = (WebSearchPlugin*) control->data;
    gchar* value = NULL;

    xmlNewTextChild(parent, NULL, (const xmlChar*) "browserCommand", 
            (xmlChar*) websearch->browser_command);
    
    value = g_strdup_printf("%d", websearch->engines->selected);
    xmlNewTextChild(parent, NULL, (const xmlChar*) "selectedEngine", 
            (xmlChar*) value);
    g_free(value);
}

static OptionsInput* options_input_new(WebSearchPlugin* websearch)
{
    SearchEngines* engines = websearch->engines;
    OptionsInput* input = g_new(OptionsInput, 1);
    int i = 0;
    
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

static void apply_options_callback(GtkWidget* widget, gpointer user_data)
{
    OptionsInput* input = (OptionsInput*) user_data;
    WebSearchPlugin* websearch = input->websearch;

    g_free(websearch->browser_command);
    websearch->browser_command = 
            g_strdup(gtk_entry_get_text(GTK_ENTRY(input->browser_entry)));

    websearch->engines->selected = 
            gtk_combo_box_get_active(GTK_COMBO_BOX(input->engine_combo));

    g_free(input);
}

static void websearch_create_options(Control* control, GtkContainer* container,
        GtkWidget *done)
{
    WebSearchPlugin* websearch = (WebSearchPlugin*) control->data;
    GtkWidget* layout = gtk_table_new(2, 2, FALSE);
    GtkWidget* label = NULL;
    OptionsInput* input = options_input_new(websearch);

    gtk_container_add(GTK_CONTAINER(container), layout);

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
    
    g_signal_connect(done, "clicked",
            G_CALLBACK(apply_options_callback), input);
    
    gtk_widget_show_all(layout);
}

static void websearch_set_size(Control* control, int size)
{
}

G_MODULE_EXPORT void xfce_control_class_init(ControlClass* cc)
{
    xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
    
    cc->name = "websearch";
    cc->caption = _("WebSearch");
    
    cc->create_control = (CreateControlFunc) websearch_create_control;
    cc->free = websearch_free;
    
    cc->attach_callback = websearch_attach_callback;
    
    cc->read_config = websearch_read_config;
    cc->write_config = websearch_write_config;
    cc->create_options = websearch_create_options;
    
    cc->set_size = websearch_set_size;
}

XFCE_PLUGIN_CHECK_INIT
