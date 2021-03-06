/*
 * Copyright (c) 2003 Eduard Roccatello (master@spine-group.org)
 *
 * Based on xfce4-sample-plugin:
 * Copyright (c) 2003 Benedikt Meurer <benedikt.meurer@unix-ag.uni-siegen.de>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <glib.h>

#include <libxfce4util/i18n.h>
#include <libxfcegui4/dialogs.h>
#include <panel/plugins.h>
#include <panel/xfce.h>

#define MAXHISTORY 6

typedef struct
{
	GtkWidget   *ebox;
	GtkWidget   *button;
    GtkWidget   *img;
    GString     *content[MAXHISTORY];
    guint       iter;
} t_clipman;

typedef struct
{
    t_clipman  *clip;
    guint       idx;
} t_action;

static GtkClipboard *primaryClip, *defaultClip;

static void resetTimer (gpointer data);
static gboolean isThere (t_clipman *clip, gchar *txt);
static gchar* filterLFCR (gchar *txt);

static gboolean isThere (t_clipman *clip, gchar *txt)
{
    gint i;
    for (i=0; i<MAXHISTORY; i++) {
        if (strcmp(clip->content[i]->str, txt) == 0)
            return TRUE;
    }
    return FALSE;
}

static gchar* filterLFCR (gchar *txt)
{
    guint i = 0;
    while (txt[i] != '\0') {
        if (txt[i] == '\n' || txt[i] == '\r' || txt[i] == '\t')
            txt[i] = ' ';
        i++;
    }
    return txt;
}

static void
clicked_menu (GtkWidget *widget, gpointer data)
{
    t_action *act = data;
    gtk_clipboard_set_text(defaultClip, act->clip->content[act->idx]->str, -1);
    gtk_clipboard_set_text(primaryClip, act->clip->content[act->idx]->str, -1);
}

static void
clicked_cb(GtkWidget *button, gpointer data)
{
    GtkMenu    *menu = NULL;
    GtkWidget  *mi;
    t_clipman  *clipman = data;
    t_action   *action = NULL;
    gboolean    hasOne = FALSE;
    guint       i;

    menu = GTK_MENU(gtk_menu_new());

    mi = gtk_menu_item_new_with_label (N_("Clipboard History"));
    gtk_widget_show (mi);
    gtk_widget_set_sensitive (mi, FALSE);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

    mi = gtk_separator_menu_item_new ();
    gtk_widget_show (mi);
    gtk_widget_set_sensitive (mi, FALSE);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

    for (i=0; i < MAXHISTORY; i++) {
        if (clipman->content[i]->str != NULL && (strcmp(clipman->content[i]->str, "") != 0)) {
            mi = gtk_menu_item_new_with_label (g_strdup_printf("%d. %s", i+1, filterLFCR(g_strndup(clipman->content[i]->str, 20))));
            gtk_widget_show (mi);
            action = g_new(t_action, 1);
            action->clip = clipman;
            action->idx = i;
            g_signal_connect (G_OBJECT (mi), "activate", G_CALLBACK (clicked_menu), (gpointer)action);
            gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
            hasOne = TRUE;
        }
    }

    if (!hasOne) {
        mi = gtk_menu_item_new_with_label (N_("< Clipboard Empty >"));
        gtk_widget_show (mi);
        gtk_widget_set_sensitive (mi, FALSE);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    }

    gtk_menu_popup (menu, NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
}

static void checkClip (t_clipman *clipman) {
    gchar *txt = NULL;

    /* Check for text in X clipboard */
    txt = gtk_clipboard_wait_for_text (primaryClip);

    if (txt != NULL && !isThere(clipman, txt)) {
            g_string_assign(clipman->content[clipman->iter], txt);
            if (clipman->iter < (MAXHISTORY - 1))
                clipman->iter++;
            else
                clipman->iter = 0;
    }
    if (txt != NULL && txt) {
        g_free(txt);
        txt = NULL;
    }

    /* Check for text in default clipboard */
    txt = gtk_clipboard_wait_for_text (defaultClip);
    if (txt != NULL && !isThere(clipman, txt)) {
            g_string_assign(clipman->content[clipman->iter], txt);
            if (clipman->iter < (MAXHISTORY - 1))
                clipman->iter++;
            else
                clipman->iter = 0;
    }
    if (txt != NULL && txt) {
        g_free(txt);
        txt = NULL;
    }
}

static t_clipman *
clipman_new(void)
{
	t_clipman *clipman;
    gint       i;

	clipman = g_new(t_clipman, 1);

	clipman->ebox = gtk_event_box_new();
	gtk_widget_show(clipman->ebox);

	clipman->button = gtk_button_new();
    gtk_button_set_relief (GTK_BUTTON(clipman->button), GTK_RELIEF_NONE);
    gtk_widget_show(clipman->button);

	gtk_container_add(GTK_CONTAINER(clipman->ebox), clipman->button);

    clipman->img = gtk_image_new_from_stock ("gtk-paste", GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (clipman->img);
    gtk_container_add (GTK_CONTAINER (clipman->button), clipman->img);

    /* Element to be modified */
    clipman->iter = 0;

    for (i=0; i<MAXHISTORY; i++) {
        clipman->content[i] = g_string_new("");
    }
    defaultClip = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
    primaryClip = gtk_clipboard_get (GDK_SELECTION_PRIMARY);

    checkClip(clipman);
    g_timeout_add_full(G_PRIORITY_DEFAULT, 512, (GSourceFunc)checkClip, clipman, (GDestroyNotify)resetTimer);
    g_signal_connect(clipman->button, "clicked", G_CALLBACK(clicked_cb), clipman);

	return(clipman);
}

static void resetTimer (gpointer data)
{
#ifdef DEBUG
    printf("Timer is dead!\n");
#endif
    g_timeout_add_full(G_PRIORITY_DEFAULT, 512, (GSourceFunc)checkClip, data, (GDestroyNotify)resetTimer);
}

static gboolean
clipman_control_new(Control *ctrl)
{
	t_clipman *clipman;

	clipman = clipman_new();

	gtk_container_add(GTK_CONTAINER(ctrl->base), clipman->ebox);

	ctrl->data = (gpointer)clipman;
	ctrl->with_popup = FALSE;

	gtk_widget_set_size_request(ctrl->base, -1, -1);

	return(TRUE);
}

static void
clipman_free(Control *ctrl)
{
    t_clipman *clipman;
    GtkItemFactory  *ifactory;
    gint i;

	g_return_if_fail(ctrl != NULL);
	g_return_if_fail(ctrl->data != NULL);

	clipman = (t_clipman *)ctrl->data;

    // FIX: Freeing items makes the panel crash
    /*
    for (i=0; i<MAXHISTORY; i++) {
        if (clipman->content[i] != NULL && clipman->content[i])
            g_string_free(clipman->content[i], FALSE);
    }
    
	g_free(clipman);*/
}

static void
clipman_read_config(Control *ctrl, xmlNodePtr parent)
{
	/* do something useful here */
}

static void
clipman_write_config(Control *ctrl, xmlNodePtr parent)
{
	/* do something useful here */
}

static void
clipman_attach_callback(Control *ctrl, const gchar *signal, GCallback cb,
		gpointer data)
{
	t_clipman *clipman;

	clipman = (t_clipman *)ctrl->data;
	g_signal_connect(clipman->ebox, signal, cb, data);
	g_signal_connect(clipman->button, signal, cb, data);
}

static void
clipman_set_size(Control *ctrl, int size)
{
	/* do the resize */
}

/* options dialog */
static void
create_options (Control *ctrl, GtkContainer *con, GtkWidget *done)
{
}

/* initialization */
G_MODULE_EXPORT void
xfce_control_class_init(ControlClass *cc)
{
	/* these are required */
	cc->name		= "clipboard";
	cc->caption		= _("Clipboard Manager");

	cc->create_control	= (CreateControlFunc)clipman_control_new;

	cc->free		= clipman_free;
	cc->attach_callback	= clipman_attach_callback;

	/* options; don't define if you don't have any ;) */
	//cc->read_config		= clipman_read_config;
	//cc->write_config	= clipman_write_config;
	//cc->create_options	= clipman_create_options;

	/* Don't use this function at all if you want xfce to
	 * do the sizing.
	 * Just define the set_size function to NULL, or rather, don't 
	 * set it to something else.
	 */
	cc->set_size		= clipman_set_size;

	/* unused in the clipman:
	 * ->set_orientation
	 * ->set_theme
	 */
	 
}

/* required! defined in panel/plugins.h */
XFCE_PLUGIN_CHECK_INIT
