/*
 * gtk-gui-utils.c
 * Copyright (C) Carl Philipp Klemm 2021 <carl@uvos.xyz>
 * 
 * gtk-gui-utils.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * gtk-gui-utils.c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gtk-gui-utils.h"
#include "types.h"
#include "comm.h"

static GtkListStore *gtk_gui_new_model(void)
{
	return gtk_list_store_new(GTK_UI_MOD_NUM_COLS, G_TYPE_STRING, G_TYPE_STRING,
							  G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING);
}

GtkTreeModel *gtk_gui_new_model_from_calls(GList *calls)
{
	GtkListStore *store = gtk_gui_new_model();

	GtkTreeIter iter;

	for(GList *element = calls; element; element = element->next) {
		CallProperties *call = element->data;
		gtk_list_store_append(store, &iter);
		char *timestr = gtk_gui_date_to_new_string(call->start_time);
		CommBackend *backend = sphone_comm_get_backend(call->backend);
		gtk_list_store_set(store, &iter,
		              GTK_UI_MOD_NAME, call->contact && call->contact->name ? call->contact->name : "<unknown>",
		              GTK_UI_MOD_LINE_ID, call->line_identifier,
		              GTK_UI_MOD_TIME, timestr,
		              GTK_UI_MOD_TEXT, NULL,
		              GTK_UI_MOD_BACKEND, call->backend,
		              GTK_UI_MOD_BACKEND_STR, backend ? backend->name : "unknown", -1);
		g_free(timestr);
	}
	return GTK_TREE_MODEL(store);
}

GtkTreeModel *gtk_gui_new_model_from_messages(GList *messages)
{
	GtkListStore *store = gtk_gui_new_model();

	GtkTreeIter iter;

	for(GList *element = messages; element; element = element->next) {
		MessageProperties *msg = element->data;
		gtk_list_store_append(store, &iter);
		char *timestr = gtk_gui_time_to_new_string(msg->time);
		CommBackend *backend = sphone_comm_get_backend(msg->backend);
		gtk_list_store_set(store, &iter,
		              GTK_UI_MOD_NAME, msg->contact && msg->contact->name ? msg->contact->name : "<unknown>",
		              GTK_UI_MOD_LINE_ID, msg->line_identifier,
		              GTK_UI_MOD_TIME, timestr,
		              GTK_UI_MOD_TEXT, msg->text,
		              GTK_UI_MOD_BACKEND, msg->backend,
		              GTK_UI_MOD_BACKEND_STR, backend ? backend->name : "unknown", -1);
		g_free(timestr);
	}
	return GTK_TREE_MODEL(store);
}

GtkTreeModel *gtk_gui_new_model_from_contacts(GList *contacts)
{
	GtkListStore *store = gtk_gui_new_model();

	GtkTreeIter iter;

	for(GList *element = contacts; element; element = element->next) {
		Contact *contact = element->data;
		contact_print(contact, __func__);
		gtk_list_store_append(store, &iter);
		CommBackend *backend = sphone_comm_get_backend(contact->backend);
		gtk_list_store_set(store, &iter,
		              GTK_UI_MOD_NAME, contact->name ?: "<unknown>" ,
		              GTK_UI_MOD_LINE_ID, contact->line_identifier,
		              GTK_UI_MOD_TIME, NULL,
		              GTK_UI_MOD_TEXT, NULL,
		              GTK_UI_MOD_BACKEND, contact->backend,
		              GTK_UI_MOD_BACKEND_STR, backend ? backend->name : "unknown", -1);
	}
	return GTK_TREE_MODEL(store);
}

char *gtk_gui_date_to_new_string(time_t time)
{
	char *str = g_malloc(256);
	if(strftime(str, 256, "%m-%d-%Y %H:%M:%S", localtime(&time)) == 0)
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
