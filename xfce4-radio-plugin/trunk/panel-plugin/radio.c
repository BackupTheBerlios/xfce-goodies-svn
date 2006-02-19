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

#ifdef ENABLE_NLS
#include <libintl.h>
#endif

#include <ctype.h>

#include <radio.h>

#include <gtk/gtk.h>

// #include <libxfce4util/i18n.h>
#include <libxfcegui4/dialogs.h>
#include <panel/global.h>
#include <panel/controls.h>
#include <panel/plugins.h>
#include <panel/xfce_support.h>

static gboolean radio_control_new(Control *ctrl) {
	return TRUE;
}

static void radio_free(Control *ctrl) { }
static void radio_read_config(Control *ctrl, xmlNodePtr parent) { }
static void radio_write_config(Control *ctrl, xmlNodePtr parent) { }
static void radio_set_size(Control *ctrl, int size) { }
static void radio_attach_callback(Control *ctrl, const gchar *signal,
						GCallback cb, gpointer data) {}
static void radio_set_orientation(Control *ctrl, int orientation) { }
static void radio_create_options(Control *ctrl, GtkContainer *container,
							GtkWidget *done) { }

G_MODULE_EXPORT void xfce_control_class_init(ControlClass *cc) {
	cc->name = "radio";
	cc->caption = _("Radio player");

	cc->create_control = (CreateControlFunc) radio_control_new;

	cc->free = radio_free;
	cc->read_config = radio_read_config;
	cc->write_config = radio_write_config;
	cc->attach_callback = radio_attach_callback;

	cc->set_size = radio_set_size;
	cc->set_orientation = radio_set_orientation;
	cc->create_options = radio_create_options;
}

XFCE_PLUGIN_CHECK_INIT
