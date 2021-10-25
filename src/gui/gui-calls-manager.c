/*
 * sphone
 * Copyright (C) Ahmed Abdel-Hamid 2010 <ahmedam@mail.usa.com>
 * 
 * sphone is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * sphone is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtk/gtk.h>
#include <signal.h>

#ifdef ENABLE_LIBHILDON
#include <hildon/hildon-gtk.h>
#endif

#include "sphone-log.h"
#include "gui-calls-manager.h"
#include "gui-contact-view.h"
#include "store.h"
#include "datapipes.h"
#include "types.h"

struct {
	GtkWidget *main_window;
	GtkListStore *dials_store;
	GtkWidget *dials_view;
	GtkWidget *answer_waiting_button;
	GtkWidget *answer_button;
	GtkWidget *hangup_button;
	GtkWidget *activate_button;
	GtkWidget *mute_button;
	GtkWidget *speaker_button;
	GtkWidget *handset_button;
} g_calls_manager;

enum{
	GUI_CALLS_COLUMN_CALL,
	GUI_CALLS_COLUMN_STATUS,
	GUI_CALLS_COLUMN_DIAL,
	GUI_CALLS_COLUMN_DESC,
	GUI_CALLS_COLUMN_PHOTO,
};

static void gui_calls_select_callback(void);
static void gui_calls_double_click_callback(void);
static void gui_calls_utils_add_call(const CallProperties *call);
static void gui_calls_new_call_callback(gconstpointer data, void *object);
static void gui_calls_call_status_callback(gconstpointer data, void *object);
static void gui_calls_utils_delete_call(const CallProperties *call);
static void gui_calls_utils_update_call(const CallProperties *call);
static void gui_calls_answer_callback(void);
static void gui_calls_hangup_callback(void);
static void gui_calls_answer_waiting_callback(void);
static void gui_calls_activate_callback(void);
static void gui_calls_mute_callback(void);
static void gui_calls_speaker_callback(void);
static void gui_calls_handset_callback(void);
static void gui_calls_audio_route_trigger(gconstpointer data, gpointer user_data);

static gboolean return_true(void)
{
	return TRUE;
}

void gui_calls_manager_init(void)
{
	g_calls_manager.main_window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(g_calls_manager.main_window),"Active Calls");
	gtk_window_set_deletable(GTK_WINDOW(g_calls_manager.main_window),FALSE);
	gtk_window_set_default_size(GTK_WINDOW(g_calls_manager.main_window),400,220);
	GtkWidget *v1=gtk_vbox_new(FALSE,0);
	GtkWidget *s = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (s),
		       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	g_calls_manager.dials_view = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(g_calls_manager.dials_view),FALSE);
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	
#ifdef ENABLE_LIBHILDON
	hildon_gtk_window_set_portrait_flags(GTK_WINDOW(g_calls_manager.main_window), HILDON_PORTRAIT_MODE_REQUEST);
#endif

	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new_with_attributes("Photo", renderer, "pixbuf", GUI_CALLS_COLUMN_PHOTO, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(g_calls_manager.dials_view), column);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	column = gtk_tree_view_column_new_with_attributes("Dial", renderer, "text", GUI_CALLS_COLUMN_DESC, NULL);
	gtk_tree_view_column_set_expand(column,TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(g_calls_manager.dials_view), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Status", renderer, "text", GUI_CALLS_COLUMN_STATUS, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(g_calls_manager.dials_view), column);

	g_calls_manager.dials_store = gtk_list_store_new(5, G_TYPE_POINTER, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_PIXBUF);

	gtk_tree_view_set_model(GTK_TREE_VIEW(g_calls_manager.dials_view), GTK_TREE_MODEL(g_calls_manager.dials_store));

	GtkWidget *h1=gtk_hbox_new(FALSE, 0);
	GtkWidget *h2=gtk_hbox_new(FALSE, 0);
	
	gtk_container_add(GTK_CONTAINER(s), g_calls_manager.dials_view);
	gtk_container_add(GTK_CONTAINER(v1), s);
	gtk_box_pack_start(GTK_BOX(v1), h1, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(v1), h2, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(g_calls_manager.main_window), v1);

	gtk_widget_show_all(v1);

	g_calls_manager.answer_button=gtk_button_new_with_label("\nAnswer\n");
	g_calls_manager.answer_waiting_button=gtk_button_new_with_label("\nAnswer\n");
	g_calls_manager.activate_button=gtk_button_new_with_label("\nActivate\n");
	g_calls_manager.hangup_button=gtk_button_new_with_label("\nHangup\n");
	g_calls_manager.mute_button=gtk_button_new_with_label("\nMute ringing\n");
	g_calls_manager.speaker_button=gtk_button_new_with_label("\nSpeaker\n");
	g_calls_manager.handset_button=gtk_button_new_with_label("\nHandset\n");
	gtk_container_add(GTK_CONTAINER(h1), g_calls_manager.activate_button);
	gtk_container_add(GTK_CONTAINER(h1), g_calls_manager.answer_button);
	gtk_container_add(GTK_CONTAINER(h1), g_calls_manager.answer_waiting_button);
	gtk_container_add(GTK_CONTAINER(h1), g_calls_manager.hangup_button);
	gtk_container_add(GTK_CONTAINER(h2), g_calls_manager.mute_button);
	gtk_container_add(GTK_CONTAINER(h2), g_calls_manager.speaker_button);
	gtk_container_add(GTK_CONTAINER(h2), g_calls_manager.handset_button);

	g_signal_connect(G_OBJECT(g_calls_manager.dials_view),"cursor-changed", G_CALLBACK(gui_calls_select_callback),NULL);
	g_signal_connect(G_OBJECT(g_calls_manager.dials_view),"row-activated", G_CALLBACK(gui_calls_double_click_callback),NULL);
	g_signal_connect(G_OBJECT(g_calls_manager.activate_button),"clicked", G_CALLBACK(gui_calls_activate_callback),NULL);
	g_signal_connect(G_OBJECT(g_calls_manager.answer_button),"clicked", G_CALLBACK(gui_calls_answer_callback),NULL);
	g_signal_connect(G_OBJECT(g_calls_manager.answer_waiting_button),"clicked", G_CALLBACK(gui_calls_answer_waiting_callback),NULL);
	g_signal_connect(G_OBJECT(g_calls_manager.hangup_button),"clicked", G_CALLBACK(gui_calls_hangup_callback),NULL);
	g_signal_connect(G_OBJECT(g_calls_manager.mute_button),"clicked", G_CALLBACK(gui_calls_mute_callback),NULL);
	g_signal_connect(G_OBJECT(g_calls_manager.speaker_button),"clicked", G_CALLBACK(gui_calls_speaker_callback),NULL);
	g_signal_connect(G_OBJECT(g_calls_manager.handset_button),"clicked", G_CALLBACK(gui_calls_handset_callback),NULL);
	g_signal_connect(G_OBJECT(g_calls_manager.main_window),"delete-event", G_CALLBACK(return_true),NULL);

	append_trigger_to_datapipe(&audio_route_pipe, gui_calls_audio_route_trigger, NULL);
	append_trigger_to_datapipe(&call_new_pipe, gui_calls_new_call_callback, NULL);
	append_trigger_to_datapipe(&call_properties_changed_pipe, gui_calls_call_status_callback, NULL);
}

void gui_calls_manager_exit(void)
{
	remove_trigger_from_datapipe(&audio_route_pipe, gui_calls_audio_route_trigger);
	remove_trigger_from_datapipe(&call_new_pipe, gui_calls_new_call_callback);
	remove_trigger_from_datapipe(&call_properties_changed_pipe, gui_calls_call_status_callback);
}

static void gui_calls_update_global_status(void)
{
	if(datapipe_get_last_data_int(&call_mode_pipe) == SPHONE_MODE_RINGING) {
		gtk_widget_show(g_calls_manager.mute_button);
	} else {
		gtk_widget_hide(g_calls_manager.mute_button);
	}

	if(datapipe_get_last_data_int(&call_mode_pipe) == SPHONE_MODE_INCALL ||
		datapipe_get_last_data_int(&call_mode_pipe) == SPHONE_MODE_INCALL_NO_ROUTE) {
		if(datapipe_get_last_data_int(&audio_route_pipe) == SPHONE_AUDIO_ROUTE_SPEAKER)
			gtk_widget_hide(g_calls_manager.speaker_button);
		else
			gtk_widget_show(g_calls_manager.speaker_button);

		if(datapipe_get_last_data_int(&audio_route_pipe) == SPHONE_AUDIO_ROUTE_HANDSET)
			gtk_widget_hide(g_calls_manager.handset_button);
		else
			gtk_widget_show(g_calls_manager.handset_button);
	}
}

static void gui_calls_audio_route_trigger(gconstpointer data, gpointer user_data)
{
	(void)user_data;
	(void)data;
	gui_calls_update_global_status();
}

static CallProperties *gui_calls_find_call(const CallProperties *call, GtkTreeIter *iter)
{
	GValue value = {0};
	do {
		gtk_tree_model_get_value(GTK_TREE_MODEL(g_calls_manager.dials_store),iter, GUI_CALLS_COLUMN_CALL, &value);
		CallProperties *icall = g_value_get_pointer(&value);
		if(call_properties_comp(call, icall)){
			return icall;
		}
		g_value_unset(&value);
	} while(gtk_tree_model_iter_next(GTK_TREE_MODEL(g_calls_manager.dials_store), iter));
	sphone_log(LL_ERR, "%s: got invalid call %p", __func__, call);
	return NULL;
}

static void gui_calls_call_status_callback(gconstpointer data, void *object)
{
	const CallProperties *call = (const CallProperties*)data;
	GtkTreeIter iter;
	(void)object;

	sphone_log(LL_DEBUG, "%s: Update call %s %s", __func__, call->line_identifier, sphone_get_state_string(call->state));

	if(call->state == SPHONE_CALL_DISCONNECTED) {
		if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(g_calls_manager.dials_store), &iter))
			return;
		CallProperties *icall = gui_calls_find_call(call, &iter);
		if(icall && !icall->awnserd){
			//TODO: replace notification_add("missed_call.png",gui_history_calls);
			sphone_log(LL_DEBUG, "%s: call ended unwanserd %s", __func__, call->line_identifier);
		}
		gui_calls_utils_delete_call(call);
	} else {
		gui_calls_utils_update_call(call);
	}

	gui_calls_update_global_status();
}

static void gui_calls_new_call_callback(gconstpointer data, void *object)
{
	const CallProperties *call = (const CallProperties*)data;
	(void)object;
	
	sphone_log(LL_DEBUG, "%s: Add new call to: %s state: %s",
			   __func__, call->line_identifier, sphone_get_state_string(call->state));
	gui_calls_utils_add_call(call);

	gui_calls_update_global_status();
}

static void gui_calls_select_callback(void)
{
	sphone_log(LL_DEBUG, "%s", __func__);
	GtkTreePath *path;
	gtk_tree_view_get_cursor(GTK_TREE_VIEW(g_calls_manager.dials_view),&path,NULL);
	GtkTreeIter iter;
	GValue value = {0};
	gtk_tree_model_get_iter(GTK_TREE_MODEL(g_calls_manager.dials_store),&iter,path);
	gtk_tree_path_free(path);
	gtk_tree_model_get_value(GTK_TREE_MODEL(g_calls_manager.dials_store),&iter, GUI_CALLS_COLUMN_CALL, &value);
	CallProperties *call = (CallProperties*)g_value_get_pointer(&value);

	sphone_log(LL_DEBUG, "%s: found call %p", __func__, call);

	if(call->state == SPHONE_CALL_ACTIVE) {
		gtk_widget_hide(g_calls_manager.activate_button);
		gtk_widget_hide(g_calls_manager.answer_waiting_button);
		gtk_widget_hide(g_calls_manager.answer_button);
		gtk_widget_show(g_calls_manager.hangup_button);
	} else if(call->state == SPHONE_CALL_HELD) {
		gtk_widget_show(g_calls_manager.activate_button);
		gtk_widget_hide(g_calls_manager.answer_waiting_button);
		gtk_widget_hide(g_calls_manager.answer_button);
		gtk_widget_show(g_calls_manager.hangup_button);
	} else if(call->state == SPHONE_CALL_DIALING) {
		gtk_widget_hide(g_calls_manager.activate_button);
		gtk_widget_hide(g_calls_manager.answer_waiting_button);
		gtk_widget_hide(g_calls_manager.answer_button);
		gtk_widget_show(g_calls_manager.hangup_button);
	} else if(call->state == SPHONE_CALL_ALERTING) {
		gtk_widget_hide(g_calls_manager.activate_button);
		gtk_widget_hide(g_calls_manager.answer_waiting_button);
		gtk_widget_hide(g_calls_manager.answer_button);
		gtk_widget_show(g_calls_manager.hangup_button);
	} else if(call->state == SPHONE_CALL_INCOMING) {
		gtk_widget_hide(g_calls_manager.activate_button);
		gtk_widget_hide(g_calls_manager.answer_waiting_button);
		gtk_widget_show(g_calls_manager.answer_button);
		gtk_widget_show(g_calls_manager.hangup_button);
	} else if(call->state == SPHONE_CALL_WATING) {
		gtk_widget_hide(g_calls_manager.activate_button);
		gtk_widget_show(g_calls_manager.answer_waiting_button);
		gtk_widget_hide(g_calls_manager.answer_button);
		gtk_widget_show(g_calls_manager.hangup_button);
	} else {
		gtk_widget_hide(g_calls_manager.activate_button);
		gtk_widget_hide(g_calls_manager.answer_waiting_button);
		gtk_widget_hide(g_calls_manager.answer_button);
		gtk_widget_show(g_calls_manager.hangup_button);
	}
	
	g_value_unset(&value);
	                         
}

static void gui_calls_double_click_callback(void)
{
	sphone_log(LL_DEBUG, "%s", __func__);
	GtkTreePath *path;
	gtk_tree_view_get_cursor(GTK_TREE_VIEW(g_calls_manager.dials_view),&path,NULL);
	GtkTreeIter iter;
	GValue value={0};
	gtk_tree_model_get_iter(GTK_TREE_MODEL(g_calls_manager.dials_store),&iter,path);
	gtk_tree_path_free(path);
	gtk_tree_model_get_value(GTK_TREE_MODEL(g_calls_manager.dials_store), &iter, GUI_CALLS_COLUMN_CALL, &value);
	CallProperties *call = (CallProperties*)g_value_get_string(&value);

	if(call->contact)
		execute_datapipe(&contact_show_pipe, call->contact);
	
	g_value_unset(&value);
}

static void gui_calls_utils_delete_call(const CallProperties *call)
{
	GtkTreeIter iter;

	if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(g_calls_manager.dials_store), &iter)){
		return;
	}

	CallProperties *icall = gui_calls_find_call(call, &iter);
	if(icall){
		gtk_list_store_remove(g_calls_manager.dials_store, &iter);
		call_properties_free(icall);
	}

	// Hide the window if no active calls
	if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(g_calls_manager.dials_store), &iter)) {
		gtk_widget_hide(g_calls_manager.main_window);
	}
	else {
		GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(g_calls_manager.dials_store),&iter);
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(g_calls_manager.dials_view), path, NULL, FALSE);
		gtk_tree_path_free(path);
	}
}

static void gui_calls_utils_update_call(const CallProperties *call)
{
	GtkTreeIter iter;

	sphone_log(LL_DEBUG, "%s: try update call %s %s", __func__, call->line_identifier, sphone_get_state_string(call->state));
	if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(g_calls_manager.dials_store), &iter))
	   return;

	CallProperties *icall = gui_calls_find_call(call, &iter);
	if(icall) {
		sphone_log(LL_DEBUG, "%s: found call %p", __func__, icall);
		icall->state = call->state;
		gtk_list_store_set(g_calls_manager.dials_store, &iter, GUI_CALLS_COLUMN_STATUS, sphone_get_state_string(icall->state), -1);
		GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(g_calls_manager.dials_store), &iter);
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(g_calls_manager.dials_view), path, NULL, FALSE);
		gtk_tree_path_free(path);
		gui_calls_select_callback();
	}
}

static void gui_calls_utils_add_call(const CallProperties *call)
{
	GtkTreeIter iter;
	gchar *desc;
	GdkPixbuf *photo = NULL;

	if(call->contact && call->contact->name) {
		desc = g_strdup_printf("%s\n%s",call->contact->name, call->line_identifier);
		if(call->contact->photo) {
			photo = call->contact->photo;
			g_object_ref(G_OBJECT(photo));
		}
	} else {
		desc = g_strdup_printf("<Unknown>\n%s\n", call->line_identifier);
	}

	CallProperties *new_call = call_properties_copy(call);
	sphone_log(LL_DEBUG, "%s: register new call %p", __func__, new_call);

	gtk_list_store_append(g_calls_manager.dials_store, &iter);
	gtk_list_store_set(g_calls_manager.dials_store, &iter,
					GUI_CALLS_COLUMN_STATUS, sphone_get_state_string(call->state),
					GUI_CALLS_COLUMN_DIAL, call->line_identifier,
					GUI_CALLS_COLUMN_CALL, new_call,
					GUI_CALLS_COLUMN_DESC, desc,
					GUI_CALLS_COLUMN_PHOTO, photo, -1);
	if(photo)
		g_object_unref(G_OBJECT(photo));
	g_free(desc);

	GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(g_calls_manager.dials_store), &iter);
	gtk_tree_view_set_cursor(GTK_TREE_VIEW(g_calls_manager.dials_view), path, NULL, FALSE);
	gtk_tree_path_free(path);
	gtk_widget_show(g_calls_manager.main_window);
}

static void gui_calls_answer_callback(void)
{
	GtkTreePath *path;
	gtk_tree_view_get_cursor(GTK_TREE_VIEW(g_calls_manager.dials_view),&path,NULL);
	GtkTreeIter iter;
	GValue value = {0};
	gtk_tree_model_get_iter(GTK_TREE_MODEL(g_calls_manager.dials_store),&iter,path);
	gtk_tree_path_free(path);
	gtk_tree_model_get_value(GTK_TREE_MODEL(g_calls_manager.dials_store),&iter, GUI_CALLS_COLUMN_CALL, &value);
	CallProperties *call = (CallProperties*)g_value_get_pointer(&value);
	call->awnserd = TRUE;

	execute_datapipe(&call_accept_pipe, call);
	
	g_value_unset(&value);
}

static void gui_calls_answer_waiting_callback(void)
{
	//TODO
}

static void gui_calls_activate_callback(void)
{
	//TODO
}

static void gui_calls_mute_callback(void)
{
	execute_datapipe(&vibrate_pipe, GINT_TO_POINTER(SPHONE_VIBRATE_STOP));
	execute_datapipe(&audio_stop_pipe, NULL);
	gui_calls_update_global_status();
}

static void gui_calls_speaker_callback(void)
{
	execute_datapipe(&audio_route_pipe, GINT_TO_POINTER(SPHONE_AUDIO_ROUTE_SPEAKER));
	gui_calls_update_global_status();
}

static void gui_calls_handset_callback(void)
{
	execute_datapipe(&audio_route_pipe, GINT_TO_POINTER(SPHONE_AUDIO_ROUTE_HANDSET));
	gui_calls_update_global_status();
}

static void gui_calls_hangup_callback(void)
{	
	GtkTreePath *path;
	gtk_tree_view_get_cursor(GTK_TREE_VIEW(g_calls_manager.dials_view), &path, NULL);
	GtkTreeIter iter;
	GValue value = {0};
	gtk_tree_model_get_iter(GTK_TREE_MODEL(g_calls_manager.dials_store), &iter, path);
	gtk_tree_path_free(path);
	gtk_tree_model_get_value(GTK_TREE_MODEL(g_calls_manager.dials_store), &iter, GUI_CALLS_COLUMN_CALL, &value);
	CallProperties *call = (CallProperties*)g_value_get_pointer(&value);
	
	if(!call) {
		sphone_log(LL_ERR, "%s: failed find call", __func__);
	} else {
		sphone_log(LL_DEBUG, "%s: hangup call %p from %s", __func__, call, call->line_identifier);
		execute_datapipe(&call_hangup_pipe, call);
	}

	g_value_unset(&value);
}
