/*
 *  xfce4-minicmd-plugin
 *  Copyright (C) 2003 Biju Philip Chacko (botsie@users.sf.net)
 *  Copyright (C) 2003 Eduard Roccatello (master@spine-group.org)
 *
 *  Based on code from:
 *
 *  xfrun4
 *  Copyright (C) 2000, 2002 Olivier Fourdan (fourdan@xfce.org)
 *  Copyright (C) 2002 Jasper Huijsmans (huysmans@users.sourceforge.net)
 *  Copyright (C) 2003 Eduard Roccatello (master@spine-group.org)
 *
 *  command.c
 *  Copyright (C) Dan <daniel@ats.energo.ru>
 *
 *  xfce4-sample-plugin
 *  Copyright (c) 2003 Benedikt Meurer <benedikt.meurer@unix-ag.uni-siegen.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

/*
 *  Copyright (c) 2003 Benedikt Meurer <benedikt.meurer@unix-ag.uni-siegen.de>
 * 
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 * 
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 
 *  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 *  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 *  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4panel/xfce-panel-plugin.h>

#ifndef PATH_MAX
#define DEFAULT_LENGTH 1024
#else
#if (PATH_MAX < 1024)
#define DEFAULT_LENGTH 1024
#else
#define DEFAULT_LENGTH PATH_MAX
#endif
#endif

#define HFILE ("xfce4" G_DIR_SEPARATOR_S "xfrun_history")

#define MAXHISTORY 10

/*
 * Types
 */
typedef struct {
    gchar *command;
    gboolean in_terminal;
} XFCommand;

typedef struct {
    GtkWidget *ebox;
    GtkWidget *entry;           /* command entry */
} t_command;

/*
 * Constants
 */
const int DISPLAY_CHARS = 10;   /* Number of characters to display in entry */

/*
 * Globals
 */
static GList *History = NULL; /* Command Entry History                */
static char  *Fileman = NULL; /* Default File Manager                 */
static GList *Curr    = NULL; /* Current History Item Being Displayed */

GCompletion *complete;        /* Command Completion Structure         */
gint nComplete;               /* Number of iteration done             */

/* Load history items in *complete item for autocompletion */
GCompletion *load_completion (void) {
    GList *hitem, *hstrings;
    XFCommand *current;

    for (hitem = History, hstrings = NULL; hitem != NULL; hitem = hitem->next) {
        current = hitem->data;
        hstrings = g_list_append(hstrings,current->command);
    }
    complete = g_completion_new(NULL);
    if (hstrings != NULL) {
        g_completion_add_items(complete, hstrings);
    }
    return complete;
}

static gboolean do_run(const char *cmd, gboolean in_terminal)
{
    gchar *execute, *path;
    gboolean success;

    g_return_val_if_fail(cmd != NULL, FALSE);

    /* this is only used to prevent us to open a directory in the 
     * users's home dir with the same name as an executable,
     * e.g. evolution */
    path = g_find_program_in_path(cmd);

    /* open directory in terminal or file manager */
    if (g_file_test(cmd, G_FILE_TEST_IS_DIR) && !path) {
        if (in_terminal)
            execute = g_strconcat("xfterm4 ", cmd, NULL);
        else
            execute = g_strconcat(Fileman, " ", cmd, NULL);
    } else {
        if (in_terminal)
            execute = g_strconcat("xfterm4 -e ", cmd, NULL);
        else
            execute = g_strdup(cmd);
    }

    g_free(path);
    success = exec_command(execute);
    g_free(execute);

    return success;
}

static char *get_fileman(void)
{
    const char *var = g_getenv("FILEMAN");

    if (var && strlen(var))
        return g_strdup(var);
    else
        return g_strdup("xffm");
}

GList *get_history(void)
{
    FILE *fp;
    char *hfile = xfce_resource_lookup (XFCE_RESOURCE_CACHE, HFILE);
    char line[DEFAULT_LENGTH];
    char *check;
    GList *cbtemp = NULL;
    XFCommand *current;

    int i = 0;

    if (!hfile)
        return NULL;

    if (!(fp = fopen (hfile, "r"))) {
        g_free (hfile);

        return NULL;
    }
    
    line[DEFAULT_LENGTH - 1] = '\0';

    /* Add a single blank line at the begining */
    current = g_new0(XFCommand, 1);
    current->command = g_strdup("");
    current->in_terminal = FALSE;
    cbtemp = g_list_append(cbtemp, current);
    

    /* no more than MAXHISTORY history items */
    for (i = 0; i < MAXHISTORY && fgets(line, DEFAULT_LENGTH - 1, fp); i++) {
        if ((line[0] == '\0') || (line[0] == '\n'))
            break;

        current = g_new0(XFCommand, 1);

        if ((check = strrchr(line, '\n')))
            *check = '\0';

        if ((check = strrchr(line, ' '))) {
            *check = '\0';
            check++;
            current->in_terminal = (atoi(check) != 0);
        } else {
            current->in_terminal = FALSE;
        }

        current->command = g_strdup(line);
        cbtemp = g_list_append(cbtemp, current);
    }

    g_free(hfile);
    fclose(fp);

    return cbtemp;
}

void put_history(const char *newest, gboolean in_terminal, GList * cb)
{
    FILE *fp;
    char *hfile = xfce_resource_save_location (XFCE_RESOURCE_CACHE, HFILE, TRUE);
    GList *node;
    int i;

    if (!(fp = fopen(hfile, "w"))) {
        g_warning(_
                  ("xfce4-minicmd-plugin: Could not write history to file %s\n"),
                  hfile);
        g_free(hfile);
        return;
    }

    fprintf(fp, "%s %d\n", newest, in_terminal);
    i = 1;

    for (node = cb; node != NULL && i < MAXHISTORY; node = node->next) {
        XFCommand *current = (XFCommand *) node->data;

        if (current->command && strlen(current->command) &&
            (strcmp(current->command, newest) != 0)) {
            fprintf(fp, "%s %d\n", current->command, current->in_terminal);
            i++;
        }
    }

    fclose(fp);
    g_free(hfile);
}

static void free_hitem(XFCommand * hitem)
{
    DBG("Freeing command line");
    g_free(hitem->command);

    DBG("Freeing Data Structure");
    g_free(hitem);
}

static void free_history(GList *history)
{
    GList *tmp;
    XFCommand *hitem;

    tmp = History;
    while(tmp) {
        hitem = (XFCommand *)tmp->data;
        DBG("Freeing Item: %s", hitem->command);
        free_hitem(hitem);
        DBG("Freed Item");
        tmp->data = NULL;
        tmp = g_list_next(tmp);
    }
    DBG("Freeing List");
    g_list_free(history);
    return;
}

static void scroll_history(gboolean forward, gint count) 
{
    int n      = 0;    /* counter */
    GList *tmp = NULL;
    
    if (History) {
        tmp = Curr ? Curr : History;
        
        if (forward) {
            for (n = 0; (n < count) && tmp; n++) {
                tmp = g_list_next(tmp);
            } 
        } else {
            for (n = 0; (n < count) && tmp; n++) {
                tmp = g_list_previous(tmp);
            } 
        }

        Curr = tmp ? tmp : Curr;
    }
    return;
}
    

static gboolean entry_buttonpress_cb(GtkWidget *entry, GdkEventButton *event, gpointer data)
{
	static Atom atom = 0;
	GtkWidget *toplevel = gtk_widget_get_toplevel (entry);

	if (event->button != 3 && toplevel && toplevel->window) {
		XClientMessageEvent xev;

		if (G_UNLIKELY(!atom))
			atom = XInternAtom (GDK_DISPLAY(), "_NET_ACTIVE_WINDOW", FALSE);

		xev.type = ClientMessage;
		xev.window = GDK_WINDOW_XID (toplevel->window);
		xev.message_type = atom;
		xev.format = 32;
		xev.data.l[0] = 0;
		xev.data.l[1] = 0;
		xev.data.l[2] = 0;
		xev.data.l[3] = 0;	
		xev.data.l[4] = 0;	

		XSendEvent (GDK_DISPLAY (), GDK_ROOT_WINDOW (), False,
					StructureNotifyMask, (XEvent *) & xev);

                gtk_widget_grab_focus (entry);
	}

        return FALSE;
}


static gboolean entry_keypress_cb(GtkWidget *entry, GdkEventKey *event, gpointer user_data)
{
    static gboolean terminal = FALSE; /* Run in a terminal?         */
    gboolean selected        = FALSE; /* Is there any selection?    */
    const gchar *cmd         = NULL;  /* command line to execute    */
    const gchar *prefix      = NULL;  /* common prefix              */
    XFCommand *hitem         = NULL;  /* history item data          */
    GList *similar           = NULL;  /* list of similar commands   */
    gint selstart;                    /* selection start            */
    gint i;                           /* just a counter             */
    gint len;                         /* command length             */


    switch (event->keyval) {
        case GDK_Down:
            scroll_history(TRUE,1);
            if (Curr) {
                hitem = (XFCommand *) Curr->data;
                terminal = hitem->in_terminal;
                gtk_entry_set_text(GTK_ENTRY(entry), hitem->command);
            }
            return TRUE;
        case GDK_Up:
            scroll_history(FALSE,1);
            if (Curr) {
                hitem = (XFCommand *) Curr->data;
                terminal = hitem->in_terminal;
                gtk_entry_set_text(GTK_ENTRY(entry), hitem->command);
            }
            return TRUE;
        case GDK_Return:
            cmd = gtk_entry_get_text(GTK_ENTRY(entry));
            
            if ((event->state) & GDK_CONTROL_MASK) {
                terminal = TRUE;
            }

            if (do_run(cmd, terminal)) {
                put_history(cmd, terminal, History);      /* save this cmdline to history       */
                free_history(History);                    /* Delete current history             */
                g_completion_free(complete);              /* Free completion items              */
                History  = get_history();                 /* reload modified history            */
                complete = load_completion();             /* Reload completion items            */
                Curr     = NULL;                          /* reset current history item pointer */
                terminal = FALSE;                         /* Reset run in term flag             */
                gtk_entry_set_text(GTK_ENTRY(entry), ""); /* clear the entry                    */
            }
            return TRUE;
        case GDK_Tab:
            cmd = gtk_entry_get_text(GTK_ENTRY(entry));
            if ((len = g_utf8_strlen(cmd, -1)) != 0) {
                /* Check for a selection */
                if ((selected = gtk_editable_get_selection_bounds(GTK_EDITABLE(entry), &selstart, NULL)) && selstart != 0) {
                    nComplete++;
                    prefix = g_strndup(cmd, selstart);
                }
                else {
                    nComplete = 0;
                    prefix = cmd;
                }
                /* Make the completion if there is items in similar */
                if ((similar = g_completion_complete(complete, prefix, NULL)) != NULL) {
                    /* Choose next element */
                    if (selected && selstart != 0) {
                        if (nComplete >= g_list_length(similar))
                            nComplete = 0;
                        for (i=0; i<nComplete; i++) {
                            if (similar->next!=NULL)
                                similar = similar->next;
                        }
                    }
                    /* Write in the entry and select the added text */
                    gtk_entry_set_text(GTK_ENTRY(entry), similar->data);
                    gtk_editable_select_region(GTK_EDITABLE(entry), (selstart == 0 ? len : selstart), -1);
                }
            }
            return TRUE;
        default:
            /* hand over to default signal handler */
            return FALSE;
    }
}

/*
 * command_new: Initialises the widgets
 */
static t_command *command_new(void)
{
    t_command *command;

    command = g_new(t_command, 1);

    command->ebox = gtk_event_box_new();
    gtk_widget_show(command->ebox);

    command->entry = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(command->entry), DISPLAY_CHARS);

    gtk_widget_show(command->entry);
    gtk_container_add(GTK_CONTAINER(command->ebox), command->entry);
    g_signal_connect(command->entry, "key-press-event", G_CALLBACK(entry_keypress_cb), command);

    g_signal_connect (command->entry, "button-press-event", G_CALLBACK(entry_buttonpress_cb), NULL);
    
    return (command);
}


static void command_free(XfcePanelPlugin *plug, gpointer data)
{
    t_command *command;
    g_return_if_fail(data != NULL);
    command = (t_command *) data;
    g_free(command);
}

static void command_construct(XfcePanelPlugin * plug)
{
    t_command *command;

    command = command_new();

    History = get_history();
    complete = load_completion();

    Fileman = get_fileman();

    gtk_container_add(GTK_CONTAINER(plug), command->ebox);
    xfce_panel_plugin_add_action_widget(plug, command->ebox);
 
    g_signal_connect(plug, "free-data", G_CALLBACK(command_free), command);
}

XFCE_PANEL_PLUGIN_REGISTER_INTERNAL(command_construct);

