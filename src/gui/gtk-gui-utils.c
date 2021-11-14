#include "gtk-gui-utils.h"
#include "types.h"

GtkTreeModel *gtk_gui_new_model_from_calls(GList *calls)
{
	GtkListStore *store = gtk_list_store_new(GTK_UI_MOD_NUM_COLS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);

	GtkTreeIter iter;

	for(GList *element = calls; element; element = element->next) {
		CallProperties *call = element->data;
		gtk_list_store_append(store, &iter);
		char *timestr = gtk_gui_date_to_new_string(call->start_time);
		gtk_list_store_set(store, &iter,
		              GTK_UI_MOD_NAME, call->contact && call->contact->name ? call->contact->name : "<unkown>",
		              GTK_UI_MOD_LINE_ID, call->line_identifier,
		              GTK_UI_MOD_TIME, timestr,
		              GTK_UI_MOD_TEXT, NULL,
		              GTK_UI_MOD_BACKEND, call->backend, -1);
		g_free(timestr);
	}
	return GTK_TREE_MODEL(store);
}

GtkTreeModel *gtk_gui_new_model_from_messages(GList *messages)
{
	GtkListStore *store = gtk_list_store_new(GTK_UI_MOD_NUM_COLS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);

	GtkTreeIter iter;

	for(GList *element = messages; element; element = element->next) {
		MessageProperties *msg = element->data;
		gtk_list_store_append(store, &iter);
		char *timestr = gtk_gui_time_to_new_string(msg->time);
		gtk_list_store_set(store, &iter,
		              GTK_UI_MOD_NAME, msg->contact && msg->contact->name ? msg->contact->name : msg->line_identifier,
		              GTK_UI_MOD_LINE_ID, msg->line_identifier,
		              GTK_UI_MOD_TIME, timestr,
		              GTK_UI_MOD_TEXT, msg->text,
		              GTK_UI_MOD_BACKEND, msg->backend, -1);
		g_free(timestr);
	}
	return GTK_TREE_MODEL(store);
}

GtkTreeModel *gtk_gui_new_model_from_contacts(GList *contacts)
{
	GtkListStore *store = gtk_list_store_new(GTK_UI_MOD_NUM_COLS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);

	GtkTreeIter iter;

	for(GList *element = contacts; element; element = element->next) {
		Contact *contact = element->data;
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
		              GTK_UI_MOD_NAME, contact->name ?: contact->line_identifier ?: "<unkown>" ,
		              GTK_UI_MOD_LINE_ID, contact->line_identifier,
		              GTK_UI_MOD_TIME, NULL,
		              GTK_UI_MOD_TEXT, NULL,
		              GTK_UI_MOD_BACKEND, contact->backend, -1);
	}
	return GTK_TREE_MODEL(store);
}

char *gtk_gui_date_to_new_string(time_t time)
{
	char *str = g_malloc(256);
	if(strftime(str, 256, "%m-%d-%Y %H:%M:%S (mon=%b)", localtime(&time)) == 0)
		str[0] = '\0';
	return str;
}

char *gtk_gui_time_to_new_string(time_t time)
{
	char *str = g_malloc(256);
	if(strftime(str, 256, "%H:%M", localtime(&time)) == 0)
		str[0] = '\0';
	return str;
}
