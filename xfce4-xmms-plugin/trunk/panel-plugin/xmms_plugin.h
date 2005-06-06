/*
 * Copyright (c) 2003 Patrick van Staveren <pvanstav@cs.wmich.edu>
 * Copyright (c) 2005 Kemal Ilgar Eroglu <kieroglu@math.washington.edu>
 * Copyright (c) 2005 Mario Streiber <mario.streiber@gmx.de>
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

#ifndef XMMS_PLUGIN_H
#define XMMS_PLUGIN_H

#define DOEXPAND TRUE
#define DOFILL   TRUE
#define PADDING  1

#define PREV (DATA_DIR "/xmms-plugin-prev.png")
#define PLAY (DATA_DIR "/xmms-plugin-play.png")
#define PAUS (DATA_DIR "/xmms-plugin-pause.png")
#define STOP (DATA_DIR "/xmms-plugin-stop.png")
#define NEXT (DATA_DIR "/xmms-plugin-next.png")

#define MAX_SCROLL_SPEED 20             /* limits and default for scroll steps per second */
#define MIN_SCROLL_SPEED 1
#define SCROLL_SPEED     10

#define MAX_SCROLL_DELAY 10             /* limits and default for delay in seconds before */
#define MIN_SCROLL_DELAY 0              /* scrolling starts                               */
#define SCROLL_DELAY     3

#define MAX_SCROLL_STEP  10             /* limits and default for scroll step width */
#define MIN_SCROLL_STEP  0
#define SCROLL_STEP      3

#define STEP_DELAY       SCROLL_DELAY * SCROLL_SPEED * SCROLL_STEP

#define MAX_TITLE_SIZE   15             /* limits and default for the song title font size */
#define MIN_TITLE_SIZE   3
#define TITLE_SIZE       9

#define TITLE_STRING     "Power Off"    /* displayed song title if xmms is not running */

/* data structure to hold all required data for the plugin */
typedef struct {
  GtkWidget      *boxMain, *pbar, *vol_pbar, *viewport, *label, *base;
  GtkTooltips    *tooltip;
  PangoAttribute *labelattr;
  PangoAttrList  *labelattrlist;
  gint           titletextsize, title_scroll_position, playlist_position;
  gint           scroll_step, scroll_speed, scroll_delay, step_delay, play_time;
  gint           xmms_session;
  guint          timeout;
  gboolean       xmmsvisible, show_mw, show_pl, show_eq, timer_reset,
                 show_scrolledtitle, quit_xmms, size_adjust, simple_title, pbar_visible, vol_pbar_visible, use_bmp;
} plugin_data;

#endif
