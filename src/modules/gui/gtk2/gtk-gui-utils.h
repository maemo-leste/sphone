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
