#include "gui-gtk-utils.h"
#include "types.h"

GtkTreeModel *gtk_gui_new_model_from_calls(GList *calls)
{
	GtkListStore *store = gtk_list_store_new(GTK_UI_MOD_NUM_COLS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

	GtkTreeIter iter;

	for(GList *element = calls; element; element = element->next) {
		CallProperties *call = element->data;
		gtk_list_store_append(store, &iter);
		char *timestr = sphone_time_to_new_string(call->start_time);
		gtk_list_store_set(store, &iter,
                      GTK_UI_MOD_NAME, call->contact && call->contact->name ? call->contact->name : "<unkown>",
                      GTK_UI_MOD_LINE_ID, call->line_identifier,
                      GTK_UI_MOD_TIME, timestr,
                      -1);
		g_free(timestr);
	}
	return GTK_TREE_MODEL(store);
}

char *sphone_time_to_new_string(time_t time)
{
	char *str = g_malloc(256);
	if(strftime(str, 256, "%m-%d-%Y %H:%M:%S (mon=%b)", localtime(&time)) == 0)
		str[0] = '\0';
	return str;
}
