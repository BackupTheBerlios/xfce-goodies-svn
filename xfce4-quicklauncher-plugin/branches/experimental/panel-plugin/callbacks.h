// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#ifndef __QCK_LAUNCHER_CALLBACKS_H__
#define __QCK_LAUNCHER_CALLBACKS_H__

#include <gtk/gtk.h>
#include "types.h"

//Global var
//extern t_qck_launcher_opt_dlg *_dlg;
//extern GtkWidget  *_icon_window;

t_qck_launcher_opt_dlg* create_qck_launcher_dlg();
void qck_launcher_opt_dlg_set_quicklauncher(t_quicklauncher *launcher);
void free_qck_launcher_dlg(GtkDialog *dialog, gint arg1, gpointer user_data);

#endif