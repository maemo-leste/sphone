#pragma once
#include <gtk/gtk.h>

enum {
  GTK_UI_MOD_NAME = 0,
  GTK_UI_MOD_LINE_ID,
  GTK_UI_MOD_TIME,
  GTK_UI_MOD_NUM_COLS
};

GtkTreeModel *gtk_gui_new_model_from_calls(GList *calls);
//GtkTreeModel *gtk_gui_new_model_from_messages(GList *calls);
char *sphone_time_to_new_string(time_t time);
