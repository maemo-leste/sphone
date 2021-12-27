/*
 * gtk-gui-utils.h
 * Copyright (C) Carl Philipp Klemm 2021 <carl@uvos.xyz>
 * 
 * gtk-gui-utils.h is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * gtk-gui-utils.h is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#include <gtk/gtk.h>

enum {
  GTK_UI_MOD_NAME = 0,
  GTK_UI_MOD_LINE_ID,
  GTK_UI_MOD_TIME,
  GTK_UI_MOD_TEXT,
  GTK_UI_MOD_BACKEND,
  GTK_UI_MOD_BACKEND_STR,
  GTK_UI_MOD_NUM_COLS
};

GtkTreeModel *gtk_gui_new_model_from_calls(GList *calls);
GtkTreeModel *gtk_gui_new_model_from_contacts(GList *contacts);
GtkTreeModel *gtk_gui_new_model_from_messages(GList *messages);
char *gtk_gui_date_to_new_string(time_t time);
char *gtk_gui_time_to_new_string(time_t time);
