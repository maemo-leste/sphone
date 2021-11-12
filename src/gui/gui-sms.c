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
#include <time.h>

#ifdef ENABLE_LIBHILDON
#include <hildon/hildon-gtk.h>
#endif

#include "comm.h"
#include "types.h"
#include "datapipe.h"
#include "datapipes.h"
#include "sphone-log.h"
#include "store.h"
#include "string.h"
#include "gui-contact-view.h"
#include "sphone-store-tree-model.h"
#include "gui-sms.h"
#include "gui-gtk-utils.h"

static void gui_sms_send_callback(GtkWidget *button, GtkWidget *main_window);
static void gui_sms_cancel_callback(GtkWidget *button, GtkWidget *main_window);
static void gui_sms_reply_callback(GtkWidget *button);

static void gui_sms_coming_callback(gconstpointer data, gpointer user_data)
{
	const MessageProperties *message = (const MessageProperties*)data;
	(void)user_data;

	gui_sms_receive_show(message);
}

static void gui_sms_open_contact_callback(GtkButton *button)
{
	gchar *dial=g_object_get_data(G_OBJECT(button),"dial");
	gui_contact_open_by_dial(dial);
}

void gtk_gui_sms_init(void)
{
	append_trigger_to_datapipe(&message_recived_pipe, gui_sms_coming_callback, NULL);
}

void gtk_gui_sms_exit(void)
{
	remove_trigger_from_datapipe(&message_recived_pipe, gui_sms_coming_callback);
}

// The model will always contain only matched entries
static gboolean gui_sms_completion_match(GtkEntryCompletion *completion, const gchar *key, GtkTreeIter *iter, gpointer user_data)
{
	(void)completion;
	(void)key;
	(void)iter;
	(void)user_data;
	return TRUE;
}

static gint gui_sms_to_changed_callback(GtkEntry *entry)
{
	const gchar *filter=gtk_entry_get_text(entry);
	GtkEntryCompletion *completion=gtk_entry_get_completion(entry);

	SphoneStoreTreeModel *dials_store;
	if(*filter)
		dials_store = sphone_store_tree_model_new(&SPHONE_STORE_TREE_MODEL_FILTER_MATCH_NAME_DIAL, filter);
	else
		dials_store = sphone_store_tree_model_new(NULL, NULL);
		
	g_object_set(G_OBJECT(dials_store), "max-rows", 25,NULL);
	gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(dials_store));
	return FALSE;
}

static void gui_sms_to_changed_callback_delayed(GtkEntry *entry)
{
	static int timeout_source_id=-1;
	if(timeout_source_id>-1)
		g_source_remove(timeout_source_id);
	timeout_source_id=g_timeout_add_seconds(1,(GSourceFunc)gui_sms_to_changed_callback, entry);
}

static void gui_sms_completion_cell_data(GtkCellLayout *cell_layout, GtkCellRenderer *cell,
										 GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data)
{
	(void)cell_layout;
	(void)data;

	GValue dial_val={0},name_val={0};
	
	gtk_tree_model_get_value(tree_model, iter, SPHONE_STORE_TREE_MODEL_COLUMN_DIAL,&dial_val);
	gtk_tree_model_get_value(tree_model, iter, SPHONE_STORE_TREE_MODEL_COLUMN_DIAL,&name_val);
	gchar *content;
	content=g_strdup_printf("%s <%s>", g_value_get_string(&name_val), g_value_get_string(&dial_val));
	g_object_set_data_full(G_OBJECT(cell), "text", content,g_free);
	g_value_unset(&dial_val);
	g_value_unset(&name_val);
}

#ifdef ENABLE_LIBHILDON
static GtkWidget *gui_sms_create_selector(void)
{
	GtkWidget *selector = hildon_touch_selector_new_text();
	GSList *list = sphone_comm_get_backends();

	for(GSList *element = list; element; element = element->next) {
		CommBackend *backend = element->data; 
		hildon_touch_selector_append_text(HILDON_TOUCH_SELECTOR(selector), backend->name);
	}

	hildon_touch_selector_set_active(HILDON_TOUCH_SELECTOR(selector), 0, 0);

	return selector;
}

#else
static GtkWidget *gui_sms_create_backend_combo(void)
{
	GtkComboBoxText *combo = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
	GSList *list = sphone_comm_get_backends();

	for(GSList *element = list; element; element = element->next) {
		CommBackend *backend = element->data; 
		gtk_combo_box_text_append_text(combo, backend->name);
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);

	return GTK_WIDGET(combo);
}
#endif

bool gtk_gui_sms_send_show(const MessageProperties *msg)
{
	GtkWidget *main_window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(main_window),"Send SMS");
	gtk_window_set_default_size(GTK_WINDOW(main_window),400,220);
	GtkWidget *v1=gtk_vbox_new(FALSE,2);
	GtkWidget *to_bar=gtk_hbox_new(FALSE,0);
	GtkWidget *actions_bar=gtk_hbox_new(FALSE,0);
	GtkWidget *to_label=gtk_label_new("To:");
	GtkWidget *to_entry=gtk_entry_new();
	GtkWidget *send_button=gtk_button_new_with_label("Send");
	GtkWidget *cancel_button=gtk_button_new_with_label("Cancel");
	GtkWidget *text_edit=gtk_text_view_new();
	GtkWidget *s = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (s),
		       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	
#ifdef ENABLE_LIBHILDON
	hildon_gtk_window_set_portrait_flags(GTK_WINDOW(main_window), HILDON_PORTRAIT_MODE_SUPPORT);
#endif

	GtkTextBuffer *text_buffer=gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_edit));

	if(msg && msg->line_identifier)
		gtk_entry_set_text(GTK_ENTRY(to_entry), msg->line_identifier);
	if(msg && msg->text)
		gtk_text_buffer_set_text(text_buffer, msg->text, -1);

	GtkCellRenderer *renderer;
	GtkEntryCompletion *completion=gtk_entry_completion_new();
	gtk_entry_completion_set_match_func(completion,gui_sms_completion_match,NULL,NULL);
	gtk_entry_completion_set_popup_completion(completion,TRUE);
	g_object_set(G_OBJECT(completion), "text-column", SPHONE_STORE_TREE_MODEL_COLUMN_DIAL,NULL);

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_cell_renderer_set_fixed_size(renderer,40,40);
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (completion),renderer, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(completion),renderer,"pixbuf", SPHONE_STORE_TREE_MODEL_COLUMN_PICTURE);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_renderer_set_fixed_size(renderer,-1,40);
	g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (completion),renderer, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(completion),renderer,"text", SPHONE_STORE_TREE_MODEL_COLUMN_NAME);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_renderer_set_fixed_size(renderer,-1,40);
	g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (completion),renderer, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(completion),renderer,"text", SPHONE_STORE_TREE_MODEL_COLUMN_DIAL);
	
	gtk_entry_set_completion (GTK_ENTRY(to_entry),completion);
	
	gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(text_edit),GTK_TEXT_WINDOW_LEFT,2);
	gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(text_edit),GTK_TEXT_WINDOW_RIGHT,2);

	gtk_box_pack_start(GTK_BOX(to_bar), to_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(to_bar), to_entry, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(actions_bar), send_button);
	gtk_container_add(GTK_CONTAINER(actions_bar), cancel_button);
	gtk_box_pack_start(GTK_BOX(v1), to_bar, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(s), text_edit);
	gtk_box_pack_start(GTK_BOX(v1), s, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(v1), actions_bar, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(main_window), v1);

	g_object_set_data(G_OBJECT(main_window),"to_entry",to_entry);
	g_object_set_data(G_OBJECT(main_window),"text_buffer",text_buffer);

	gtk_widget_show_all(main_window);
	
	g_signal_connect(G_OBJECT(send_button),"clicked", G_CALLBACK(gui_sms_send_callback),main_window);
	g_signal_connect(G_OBJECT(cancel_button),"clicked", G_CALLBACK(gui_sms_cancel_callback),main_window);
	g_signal_connect(G_OBJECT(to_entry),"changed", G_CALLBACK(gui_sms_to_changed_callback_delayed),NULL);
	gui_sms_to_changed_callback(GTK_ENTRY(to_entry));
	return true;
}

void gui_sms_receive_show(const MessageProperties *message)
{
	gchar *desc;
	gchar *time_str;
	GdkPixbuf *photo = NULL;

	if(message->contact && message->contact->name) {
		desc = g_strdup_printf("%s\n%s",message->contact->name, message->line_identifier);
		if(message->contact->photo)
			photo = message->contact->photo;
	} else {
		desc = g_strdup_printf("<Unknown>\n%s\n", message->line_identifier);
	}
	
	time_str = sphone_time_to_new_string(message->time);

	GtkWidget *main_window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(main_window),"New SMS");
	gtk_window_set_default_size(GTK_WINDOW(main_window),400,220);
	GtkWidget *v1=gtk_vbox_new(FALSE,0);
	GtkWidget *to_bar=gtk_hbox_new(FALSE,0);
	GtkWidget *actions_bar=gtk_hbox_new(TRUE,0);
	GtkWidget *photo_image=gtk_image_new_from_pixbuf(photo);
	GtkWidget *from_entry=gtk_button_new_with_label(desc);
	GtkWidget *time_label=gtk_label_new(time_str);
	GtkWidget *send_button=gtk_button_new_with_label("Reply");
	GtkWidget *cancel_button=gtk_button_new_with_label("Cancel");
	GtkWidget *text_edit=gtk_label_new(message->text);
	GtkWidget *s = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (s),
		       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_misc_set_alignment(GTK_MISC(text_edit),0.0,0.0);
	gtk_misc_set_padding(GTK_MISC(text_edit),2,2);
	gtk_button_set_relief(GTK_BUTTON(from_entry),GTK_RELIEF_NONE);
	gtk_button_set_alignment(GTK_BUTTON(from_entry),0,0.5);
	//gtk_widget_set_can_focus(from_entry,FALSE);
	g_object_set(G_OBJECT(from_entry),"can-focus",FALSE,NULL);

	gtk_box_pack_start(GTK_BOX(to_bar), photo_image, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(to_bar), from_entry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(to_bar), time_label, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(actions_bar), send_button);
	gtk_container_add(GTK_CONTAINER(actions_bar), cancel_button);
	gtk_box_pack_start(GTK_BOX(v1), to_bar, FALSE, FALSE, 0);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(s), text_edit);
	gtk_box_pack_start(GTK_BOX(v1), s, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(v1), actions_bar, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(main_window), v1);

	gtk_widget_show_all(main_window);

	g_object_set_data_full(G_OBJECT(from_entry), "dial", g_strdup(message->line_identifier), g_free);
	g_object_set_data_full(G_OBJECT(send_button), "dial", g_strdup(message->line_identifier), g_free);
	
	g_signal_connect(G_OBJECT(from_entry),"clicked", G_CALLBACK(gui_sms_open_contact_callback),NULL);
	g_signal_connect(G_OBJECT(send_button),"clicked", G_CALLBACK(gui_sms_reply_callback),NULL);
	g_signal_connect(G_OBJECT(cancel_button),"clicked", G_CALLBACK(gui_sms_cancel_callback),main_window);
	
	g_free(time_str);
	g_free(desc);
}

void gui_sms_send_callback(GtkWidget *button, GtkWidget *main_window)
{
	(void)button;
	GtkEntry *to_entry=g_object_get_data(G_OBJECT(main_window),"to_entry");
	GtkTextBuffer *text_buffer=g_object_get_data(G_OBJECT(main_window),"text_buffer");

	const gchar *to = gtk_entry_get_text(to_entry);
	gchar *text=NULL;
	g_object_get(G_OBJECT(text_buffer),"text", &text, NULL);
	
	if(strlen(text) > 0 && strlen(to) > 0) {
		MessageProperties *message = g_malloc0(sizeof(*message));
		message->line_identifier = g_strdup(to);
		message->text = text;
		message->backend = sphone_comm_default_backend()->id;
		message->time = time(NULL);
		execute_datapipe(&message_send_pipe, message);
		message_properties_free(message);
		gtk_widget_destroy(main_window);
	} else {
		const gchar *message =
			strlen(text) > 0 ? "Can not send message without to field" :
			"Can not send message without text";
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window), GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
		                                           GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s", message);
		g_signal_connect_swapped(dialog, "response", G_CALLBACK(gtk_widget_destroy), dialog);
		
		gtk_widget_show_all(dialog);
	}
}

void gui_sms_cancel_callback(GtkWidget *button, GtkWidget *main_window)
{
	(void)button;
	gtk_widget_destroy(main_window);
}

void gui_sms_reply_callback(GtkWidget *button)
{
	gchar *from=g_object_get_data(G_OBJECT(button),"dial");
	MessageProperties msg = {0};
	CommBackend *backend = sphone_comm_default_backend();
	
	msg.backend = backend ? backend->id : 0;
	msg.line_identifier = from;
	gtk_gui_sms_send_show(&msg);
}
