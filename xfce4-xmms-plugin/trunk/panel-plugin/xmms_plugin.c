/*
 * xfcexmms.c
 *
 * authors: Eoin Coffey <ecoffey@simla.colostate.edu>
 *          Patrick van Staveren <pvanstav@cs.wmich.edu>
 *
 * desc: a xmms plugin for the xfce4 panel ; licensed under the GPL (see COPYING in root dir)
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <sys/wait.h>

#include <gtk/gtk.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>

#include <panel/plugins.h>
#include <panel/xfce.h>

#if defined(PLAYER_XMMS)
#include <xmmsctrl.h>
#elif defined(PLAYER_BEEP)
#include <beep/beepctrl.h>
#else
#error "Unsupported media player"
#endif

#define UPDATE_TIMEOUT 500

/* GLOBALS */
static gboolean mw_shown, pl_shown, eq_shown, hidden; /* to keep track of window state */
static gboolean running; /* is xmms running? */
static GtkWidget *pl_menu, *list; /* the playlist menu and the item that contains it */
static GtkWidget *pentry; /* entry widget that collects the text to be displayed when paused */
static GtkWidget *sentry; /* entry widget that collects the text to be displayed when stopped */
static GtkWidget *show_vol_perm;
static GtkWidget *image = NULL;
static gint show_volume_timeout = 3;
static gint _timeout;
/* OPTIONS */ 
static gint remaining = 1; /* defaults to show time remaining in song ; changeable in the options dialog */
static gint volume_adjust = 3; /* adjusts the volume by three ; changeable in the options dialog */
static gchar paused_text[20] = "(Paused)"; /* what string displays when play is paused ; changeable in options dialog */
static gchar stopped_text[20] = "(Stopped)"; /* what string displays when play is stoped ; changeable in options dialog */
static gint show_volume = 1;
static gint show_volume_perm = 0;

/* our pretty little widgets */
typedef struct
{
	GtkWidget	*ebox;
	GtkWidget	*hbbox;
	GtkWidget	*menu;
	GtkWidget	*separator;
	GtkWidget	*image;
	GtkWidget	*label;
	GtkWidget	*start;
	GtkWidget	*play;
	GtkWidget	*pause;
	GtkWidget	*stop;
	GtkWidget	*eject;
	GtkWidget	*next;
	GtkWidget	*prev;
	GtkWidget	*quit;
	GtkWidget	*prefs;
	GtkTooltips	*tips;
	guint		timeout_id;
} gui_t;

/* 
  ** HELPER FUNCTIONS ** 
*/

static void start_xmms(void)
{
	g_spawn_command_line_async(PLAYER_COMMAND, NULL);
}

static gchar* build_file_path(gchar *path)
{
	return g_strdup_printf("%s/%s", DATA_DIR, path);
}

static GtkWidget* build_menu_item(gchar *impath, gchar *text, int stock)
{
	GtkWidget *item, *image;
 	gchar *path;

	if (impath != NULL) {
		item = gtk_image_menu_item_new_with_mnemonic(text);
		if (stock == 0) {
			path = build_file_path(impath);
			image = gtk_image_new_from_file(path);
			g_free(path);
			gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), image);
		} else if (stock == 1) {
			image = gtk_image_new_from_stock(impath, GTK_ICON_SIZE_MENU);
			gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), image);
		}
	} else {
		item = gtk_menu_item_new_with_mnemonic(text);
	}
	
	gtk_widget_show_all(item);
	
	return item;
}

static gchar* format_time(gint time, gint len)
{
	int milli, secs, mins;
	
	if (remaining == 1) {
		time = len - time;
	}
	
	milli = time % 1000;
	time /= 1000;
	secs = time % 60;
	time /= 60;
	mins = time % 60;

	return g_strdup_printf("(%02d:%02d)", mins, secs);
}
	 
/*
  ** CALLBACK FUNCTIONS **
*/

static void eject_clicked(GtkWidget *widget, gpointer data)
{
	xmms_remote_eject(0);
}

static void next_clicked(GtkWidget *button, gpointer data)
{
	xmms_remote_playlist_next(0);
}

static void stop_clicked(GtkWidget *button, gpointer data) 
{
	xmms_remote_stop(0);
}

static void pause_clicked(GtkWidget *button, gpointer data) 
{
	xmms_remote_pause(0); 
}

static void play_clicked(GtkWidget *button, gpointer data) 
{
	xmms_remote_play(0); 
}

static void prev_clicked(GtkWidget *button, gpointer data) 
{
	xmms_remote_playlist_prev(0);
}

static void prefs_clicked(GtkWidget *button, gpointer data)
{
	xmms_remote_show_prefs_box(0);
}

static void quit_clicked(GtkWidget *button, gpointer data) 
{
	xmms_remote_quit(0);
}

static void clear_playlist_clicked(GtkWidget *widget, gpointer data)
{
	xmms_remote_stop(0);
	xmms_remote_playlist_clear(0);
}

static void playlist_clicked(GtkWidget *widget, gpointer data)
{
	int pos = (int)data;

	xmms_remote_stop(0);	
	xmms_remote_set_playlist_pos(0, pos);
	xmms_remote_play(0);
}

static gint button_pressed(GtkWidget *widget, GdkEvent *event)
{
	GtkMenu *menu;
	GtkWidget *item;
	GdkEventButton *event_button;
	int i, len, pos;
	gchar *text;

	/* make sure we need to be here */
	g_return_val_if_fail (widget != NULL, FALSE);
	g_return_val_if_fail (GTK_IS_MENU (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	/* our "root" menu */
	menu = GTK_MENU (widget);

	if (event->type == GDK_BUTTON_PRESS) {
		event_button = (GdkEventButton *) event;
		
		/* normal left click */
		if (event_button->button == 1) {
			if (running == TRUE) {
				/* rebuild the playlist menu */
				if (pl_menu != NULL) {
					gtk_widget_destroy(pl_menu);
				}
			
				pl_menu = gtk_menu_new();
			
				len = xmms_remote_get_playlist_length(0);
				pos = xmms_remote_get_playlist_pos(0);
				
				item = build_menu_item(GTK_STOCK_CLEAR, "Clear Playlist", 1);
				g_signal_connect(item, "activate", G_CALLBACK(clear_playlist_clicked), NULL);
				gtk_menu_shell_append(GTK_MENU_SHELL(pl_menu), item);
				
				item = gtk_separator_menu_item_new();
				gtk_menu_shell_append(GTK_MENU_SHELL(pl_menu), item);
	
				for (i = 0; i < len; i++) {
					text = g_strdup_printf("%d. %s", i + 1, xmms_remote_get_playlist_title(0, i));
					if (i == pos)
						item = build_menu_item(GTK_STOCK_YES, text, 1);
					else
						item = build_menu_item(GTK_STOCK_NO, text, 1);
					g_signal_connect(item, "activate", G_CALLBACK(playlist_clicked), (gpointer)i);
					gtk_menu_shell_append(GTK_MENU_SHELL(pl_menu), item);
					g_free(text);
				}
			
				gtk_widget_show_all(pl_menu);
			
				gtk_menu_item_set_submenu(GTK_MENU_ITEM(list), pl_menu);
				
				/* popup the menu */
	  			gtk_menu_popup (menu, NULL, NULL, NULL, NULL, 
				  event_button->button, event_button->time);
				  
				/* event handled */
	  			return TRUE;
	  		}
		}
		
		/* middle click */
		if (event_button->button == 2) {
			/* launch if not running */
			if (running == FALSE) {
				start_xmms();
				mw_shown = xmms_remote_is_main_win(0);
				pl_shown = xmms_remote_is_pl_win(0);
 				eq_shown = xmms_remote_is_eq_win(0);	 				
				running = TRUE;
			}
			else { /* hide or unhide accordingly */
				if (hidden) { /* show... */
					xmms_remote_pl_win_toggle(0, pl_shown);
					xmms_remote_eq_win_toggle(0, eq_shown);
					xmms_remote_main_win_toggle(0, mw_shown);
					hidden = FALSE;
				} else { /* save what windows are up and then hide */
					mw_shown = xmms_remote_is_main_win(0);
					pl_shown = xmms_remote_is_pl_win(0);
	 				eq_shown = xmms_remote_is_eq_win(0);
	 				xmms_remote_main_win_toggle(0, FALSE);
	 				xmms_remote_pl_win_toggle(0, FALSE);
	 				xmms_remote_eq_win_toggle(0, FALSE);
	 				hidden = TRUE;
	 			}
	 		}
		}
    	}	

	/* propagate event */
	return FALSE;
}

static gboolean scroll_event(GtkWidget *widget, GdkEventScroll *event, gpointer data)
{
	int vol;
	
	if (event->type != GDK_SCROLL) 
		return FALSE;
		
	vol = xmms_remote_get_main_volume(0);
	
	if (event->direction == GDK_SCROLL_UP) {
		vol += volume_adjust;
		vol = (vol > 100) ? 100 : vol;
		xmms_remote_set_main_volume(0, vol);
	}
	
	if (event->direction == GDK_SCROLL_DOWN) {
		vol -= volume_adjust;
		vol = (vol < 0) ? 0 : vol;
		xmms_remote_set_main_volume(0, vol);
	}
	
	_timeout = show_volume_timeout * (1000/UPDATE_TIMEOUT);
	return TRUE;
}

/* this function is called by a timer */
static gboolean update_tooltip(gpointer data)
{
	gui_t *xfcexmms = (gui_t*)data;
	gchar *text, *time, *paused, *stopped, *volume;
	gint pos;
	
	/* update if we are running or not */
	running = xmms_remote_is_running(0);

	if (running == TRUE) {
		/* build the tip */
		pos = xmms_remote_get_playlist_pos(0);
		time = format_time(xmms_remote_get_output_time(0), xmms_remote_get_playlist_time(0, pos));
		paused = (xmms_remote_is_paused(0) == TRUE) ? paused_text : "";
		stopped = ((xmms_remote_is_paused(0) == FALSE) && (xmms_remote_is_playing(0) == FALSE)) ? stopped_text : "";
		if (show_volume && _timeout != 0) {
			volume = g_strdup_printf("(Vol: %d)", xmms_remote_get_main_volume(0));
			_timeout = (show_volume_perm) ? _timeout : _timeout - 1;
		} else
			volume = g_strdup("");
		text = g_strdup_printf("%d. %s %s %s%s%s", pos + 1, xmms_remote_get_playlist_title(0, pos), time, paused, stopped, volume);
		g_free(time);
		g_free(volume);
	} else {
		/* client isnt even on! */
		text = g_strdup("XMMS NOT RUNNING\n(Middle click to launch)");
	}
	
	gtk_tooltips_set_tip(GTK_TOOLTIPS(xfcexmms->tips), xfcexmms->ebox, text, NULL);
	
	g_free(text);
	
	return TRUE;
}

void apply_options(GtkWidget *button, gpointer data)
{	
	g_snprintf(paused_text, 19, gtk_editable_get_chars(GTK_EDITABLE(pentry), 0, -1));
	
	g_snprintf(stopped_text, 19, gtk_editable_get_chars(GTK_EDITABLE(sentry), 0, -1));
}

void spin_changed(GtkSpinButton *button, gpointer data)
{
	volume_adjust = gtk_spin_button_get_value_as_int(button);
}

void vol_timeout_changed(GtkSpinButton *button, gpointer data)
{
	show_volume_timeout = gtk_spin_button_get_value_as_int(button);
	_timeout =  show_volume_timeout * (1000/UPDATE_TIMEOUT);
}

void show_vol_toggled(GtkToggleButton *button, gpointer data)
{
	GtkWidget *spin = (GtkWidget*)data;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == TRUE) {
		show_volume = 1;
		gtk_widget_set_sensitive(spin, TRUE);
		gtk_widget_set_sensitive(show_vol_perm, TRUE);
	} else {
		show_volume = 0;
		gtk_widget_set_sensitive(spin, FALSE);
		gtk_widget_set_sensitive(show_vol_perm, FALSE);
	}
}

void show_vol_perm_toggled(GtkToggleButton *button, gpointer data)
{
	GtkWidget *spin = (GtkWidget*)data;
	
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == TRUE) {
		show_volume_perm = 1;
		_timeout = show_volume_timeout;
		gtk_widget_set_sensitive(spin, FALSE);
	} else {
		show_volume_perm = 0;
		gtk_widget_set_sensitive(spin, TRUE);
	}
}

void remain_toggled(GtkToggleButton *button, gpointer data)
{
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == TRUE)
		remaining = 1;
	else
		remaining = 0;
}
	
/*
  ** XFCE PLUGIN CODE **
*/

static gui_t* xfcexmms_new(void)
{
	gui_t *gui;
	
	/* initalize our globals */
	running = xmms_remote_is_running(0);
	 
	if (running == TRUE) {
        	mw_shown = xmms_remote_is_main_win(0);
	 	pl_shown = xmms_remote_is_pl_win(0);
	 	eq_shown = xmms_remote_is_eq_win(0);
	 	if (mw_shown == TRUE)
	 		hidden = FALSE;
	 	else
	 		hidden = TRUE;
	} else {
		mw_shown = TRUE;
	 	pl_shown = TRUE;
	 	eq_shown = FALSE;
	 	hidden = FALSE;
	}
	
	gui = g_new(gui_t, 1);
	
	pl_menu = NULL;
	
	/* our base widget */
	gui->ebox = gtk_event_box_new();
	
	/* build our menu */
	gui->menu = gtk_menu_new();
	
	gui->eject = build_menu_item("xmms-plugin-menu-eject.png", "Eject", 0);
	g_signal_connect(gui->eject, "activate", G_CALLBACK(eject_clicked), NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(gui->menu), gui->eject);
	
	gui->prev = build_menu_item("xmms-plugin-menu-prev.png", "Backward", 0);
	g_signal_connect(gui->prev, "activate", G_CALLBACK(prev_clicked), NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(gui->menu), gui->prev);
	
	gui->stop = build_menu_item("xmms-plugin-menu-stop.png", "Stop", 0);
	g_signal_connect(gui->stop, "activate", G_CALLBACK(stop_clicked), NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(gui->menu), gui->stop);
	
	gui->pause = build_menu_item("xmms-plugin-menu-pause.png", "Pause", 0);
	g_signal_connect(gui->pause, "activate", G_CALLBACK(pause_clicked), NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(gui->menu), gui->pause);
	
	gui->play = build_menu_item("xmms-plugin-menu-play.png", "Play", 0);
	g_signal_connect(gui->play, "activate", G_CALLBACK(play_clicked), NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(gui->menu), gui->play);
	
	gui->next = build_menu_item("xmms-plugin-menu-next.png", "Forward", 0);
	g_signal_connect(gui->next, "activate", G_CALLBACK(next_clicked), NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(gui->menu), gui->next);
	
	gui->separator = gtk_separator_menu_item_new();
	gtk_widget_show_all(gui->separator);
	gtk_menu_shell_prepend(GTK_MENU_SHELL(gui->menu), gui->separator);
	
	list = build_menu_item(GTK_STOCK_INDEX, "Playlist", 1);
	gtk_menu_shell_prepend(GTK_MENU_SHELL(gui->menu), list);
	
	gui->separator = gtk_separator_menu_item_new();
	gtk_widget_show_all(gui->separator);
	gtk_menu_shell_prepend(GTK_MENU_SHELL(gui->menu), gui->separator);
	
	gui->prefs = build_menu_item(GTK_STOCK_PREFERENCES, "Preferences", 1);
	g_signal_connect(gui->prefs, "activate", G_CALLBACK(prefs_clicked), NULL);
	gtk_menu_shell_prepend(GTK_MENU_SHELL(gui->menu), gui->prefs);
	
	gui->quit = build_menu_item(GTK_STOCK_QUIT, "Quit", 1);
	g_signal_connect(gui->quit, "activate", G_CALLBACK(quit_clicked), NULL);
	gtk_menu_shell_prepend(GTK_MENU_SHELL(gui->menu), gui->quit);
		
	gtk_widget_show_all(gui->menu);
	
	/* create the plugin interface */

	gui->label = gtk_label_new(" XMMS ");
	
	g_signal_connect_swapped(gui->ebox, "button_press_event", G_CALLBACK(button_pressed), gui->menu);
	g_signal_connect(gui->ebox, "scroll-event", G_CALLBACK(scroll_event), NULL);
	
	gtk_container_add(GTK_CONTAINER(gui->ebox), image);
	
	gui->tips = gtk_tooltips_new();
	
	update_tooltip(gui);
	
	gui->timeout_id = g_timeout_add(UPDATE_TIMEOUT, (GtkFunction)update_tooltip, gui);
	
	gtk_widget_show_all(gui->ebox);

	return (gui);
}

static gboolean xfcexmms_control_new(Control *ctrl)
{
	gui_t *xfcexmms;

	xfcexmms = xfcexmms_new();

	gtk_container_add(GTK_CONTAINER(ctrl->base), xfcexmms->ebox);

	ctrl->data = (gpointer)xfcexmms;
	ctrl->with_popup = FALSE;

	return(TRUE);
}

static void xfcexmms_set_size(Control *ctrl, int size)
{
	int s1, s2;
	gui_t *xfcexmms;
	gchar *path;
	GdkPixbuf *_image;
	
	xfcexmms = (gui_t *)ctrl->data;
	
	if (image != NULL) {
		gtk_widget_destroy(image);
	}

	path = build_file_path("xmms-plugin-menu-icon.png");
	_image = get_scaled_pixbuf(get_pixbuf_from_file(path), icon_size[size]);
	g_free(path);
	image = gtk_image_new_from_pixbuf(_image);
	
	gtk_widget_show(image);
	
	gtk_container_add(GTK_CONTAINER(xfcexmms->ebox), image);
	
	s1 = icon_size[size] + border_width;
	s2 = s1 * 0.75;
	
	if (size > SMALL)
	    gtk_widget_set_size_request(xfcexmms->ebox, s1, s2);
	else
	    gtk_widget_set_size_request(xfcexmms->ebox, s1, s1 * 0.75);
}

static void xfcexmms_free(Control *ctrl)
{
	gui_t *xfcexmms;

	g_return_if_fail(ctrl != NULL);
	g_return_if_fail(ctrl->data != NULL);

	xfcexmms = (gui_t *)ctrl->data;

	if (xfcexmms->timeout_id)
		g_source_remove(xfcexmms->timeout_id);
	
	g_free(xfcexmms);
}


static void xfcexmms_attach_callback(Control *ctrl, const gchar *signal, GCallback cb, gpointer data)
{
}

static void xfcexmms_create_options (Control *ctrl, GtkContainer *con, GtkWidget *done)
{
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *remain;
	GtkWidget *elapsed;
	GtkWidget *hbar;
	GtkWidget *spin, *spin2;
	GtkWidget *show_vol; 
	GtkAdjustment *spin_adj;
	
	vbox = gtk_vbox_new(FALSE, 2);
	
	gtk_container_add(GTK_CONTAINER(con), vbox);
	
	/* TIME CONFIG */
	label = gtk_label_new("Display time as:");
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, FALSE, 1);
	
	hbox = gtk_hbox_new(TRUE, 2);
	remain = gtk_radio_button_new_with_label(NULL, "Remaining");
	elapsed = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(remain), "Elapsed");
	
	gtk_box_pack_start(GTK_BOX(hbox), remain, TRUE, TRUE, 1);
	gtk_box_pack_start(GTK_BOX(hbox), elapsed, TRUE, TRUE, 1);
	g_signal_connect(G_OBJECT(remain), "toggled", G_CALLBACK(remain_toggled), NULL);
	
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 1);
	
	/* VOLUME CONFIG */
	hbar = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox), hbar, TRUE, TRUE, 1);
	
	hbox = gtk_hbox_new(FALSE, 2);
	
	label = gtk_label_new("Increase Volume By:");
	gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 1);
	
	spin_adj = (GtkAdjustment*)gtk_adjustment_new((gdouble)volume_adjust, 0.0, 100.0, 1.0, 5.0, 5.0);
	spin = gtk_spin_button_new(spin_adj, 1.0, 0);
	g_signal_connect(G_OBJECT(spin), "value-changed", G_CALLBACK(spin_changed), NULL);
	
	gtk_box_pack_start(GTK_BOX(hbox), spin, TRUE, FALSE, 1);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 1);
	
	show_vol = gtk_check_button_new_with_label("Display Volume");
	
	gtk_box_pack_start(GTK_BOX(vbox), show_vol, TRUE, TRUE, 1);
	
	show_vol_perm = gtk_check_button_new_with_label("Always Display");
	
	gtk_box_pack_start(GTK_BOX(vbox), show_vol_perm, TRUE, TRUE, 1);
	
	hbox = gtk_hbox_new(FALSE, 2);
	
	label = gtk_label_new("Volume Display Timeout:");
	gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 1);
	
	spin_adj = (GtkAdjustment*)gtk_adjustment_new((gdouble)show_volume_timeout, 0.0, 100.0, 1.0, 5.0, 5.0);
	spin2 = gtk_spin_button_new(spin_adj, 1.0, 0);
	g_signal_connect(G_OBJECT(spin2), "value-changed", G_CALLBACK(vol_timeout_changed), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), spin2, TRUE, FALSE, 1);
	
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 1);
	
	g_signal_connect(G_OBJECT(show_vol_perm), "toggled", G_CALLBACK(show_vol_perm_toggled), spin2);
	g_signal_connect(G_OBJECT(show_vol), "toggled", G_CALLBACK(show_vol_toggled), spin2);
	
	/* TEXT CONFIG */
	hbar = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox), hbar, TRUE, TRUE, 1);
	
	label = gtk_label_new("Paused Text:");
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, FALSE, 1);
	
	pentry = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(pentry), 19);
	gtk_entry_set_text(GTK_ENTRY(pentry), paused_text);
	gtk_box_pack_start(GTK_BOX(vbox), pentry, TRUE, TRUE, 1);
	
	label = gtk_label_new("Stopped Text:");
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, FALSE, 1);
	
	sentry = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(sentry), 19);
	gtk_entry_set_text(GTK_ENTRY(sentry), stopped_text);
	gtk_box_pack_start(GTK_BOX(vbox), sentry, TRUE, TRUE, 1);
 
 	g_signal_connect(GTK_WIDGET(done), "clicked", G_CALLBACK(apply_options), NULL);
            
	if (remaining == 1) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remain), TRUE);
	} else {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(elapsed), TRUE);
	}
	
	if (show_volume == 1) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_vol), TRUE);
	} else {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_vol), FALSE);
	}
	
	if (show_volume_perm == 1) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_vol_perm), TRUE);
		gtk_widget_set_sensitive(spin2, FALSE);
	} else {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_vol_perm), FALSE);
	}
	
	gtk_widget_show_all(GTK_WIDGET(vbox));
}

extern xmlDocPtr xmlconfig;
#define MYDATA(node) xmlNodeListGetString(xmlconfig, node->children, 1)
#define ROOT "XFCEXMMS"

static void xfcexmms_read_config(Control *control, xmlNodePtr node)
{
	xmlChar *value;
	
	if (node == NULL || node->children == NULL) {
        	return;
    	}
    	
    	for (node = node->children; node; node = node->next) {
    		if (xmlStrEqual(node->name, (const xmlChar *)ROOT)) {
    			if ((value = xmlGetProp(node, (const xmlChar *)"display_time"))) {
    				remaining = atoi(value);
    				g_free(value);
    			}
    			if ((value = xmlGetProp(node, (const xmlChar *)"volume_adjust"))) {
    				volume_adjust = atoi(value);
    				g_free(value);
    			}
    			if ((value = xmlGetProp(node, (const xmlChar *)"paused_text"))) {
    				g_snprintf(paused_text, 19, value);
    				g_free(value);
    			}
    			if ((value = xmlGetProp(node, (const xmlChar *)"stopped_text"))) {
    				g_snprintf(stopped_text, 19, value);
    				g_free(value);
    			}
    			if ((value = xmlGetProp(node, (const xmlChar*)"show_volume"))) {
    				show_volume = atoi(value);
    				g_free(value);
    			}
    			if ((value = xmlGetProp(node, (const xmlChar*)"show_volume_perm"))) {
    				show_volume_perm = atoi(value);
    				g_free(value);
    			}
    			if ((value = xmlGetProp(node, (const xmlChar*)"show_volume_timeout"))) {
    				show_volume_timeout = atoi(value);
    				g_free(value);
    			}
    			break;
    		}
    	}
}

static void xfcexmms_write_config(Control *control, xmlNodePtr parent)
{
	xmlNodePtr root;
    	gchar value[20];
    	
    	root = xmlNewTextChild(parent, NULL, ROOT, NULL);
    	
    	g_snprintf(value, 2, "%d", remaining);
    	
    	xmlSetProp(root, "display_time", value);
    	
    	g_snprintf(value, 4, "%d", volume_adjust);
    	
    	xmlSetProp(root, "volume_adjust", value);
    
    	g_snprintf(value, 19, "%s", paused_text);
    	
    	xmlSetProp(root, "paused_text", value);
    
    	g_snprintf(value, 19, "%s", stopped_text);
    	
	xmlSetProp(root, "stopped_text", value);
	
	g_snprintf(value, 2, "%d", show_volume);
	
	xmlSetProp(root, "show_volume", value);
	
	g_snprintf(value, 2, "%d", show_volume_perm);
	
	xmlSetProp(root, "show_volume_perm", value);
	
	g_snprintf(value, 4, "%d", show_volume_timeout);
	
	xmlSetProp(root, "show_volume_timeout", value);
    	    		    	
    	root = xmlNewTextChild(parent, NULL, ROOT, NULL);
}

/* initialization */
G_MODULE_EXPORT void xfce_control_class_init(ControlClass *cc)
{
	cc->name		= "xmms_plugin";
	cc->caption		= _("XMMS Control");
	cc->create_control	= (CreateControlFunc)xfcexmms_control_new;

	cc->free		= xfcexmms_free;
	cc->read_config		= xfcexmms_read_config;
    	cc->write_config 	= xfcexmms_write_config;
	cc->attach_callback	= xfcexmms_attach_callback;
	
	cc->create_options	= xfcexmms_create_options;
	
	/* although the function does nothing this still needs to be there for 
	   some reason */	
	cc->set_size		= xfcexmms_set_size;
}

XFCE_PLUGIN_CHECK_INIT
