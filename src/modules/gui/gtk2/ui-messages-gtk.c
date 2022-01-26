/*
 * sphone
 * Copyright (C) Ahmed Abdel-Hamid 2010 <ahmedam@mail.usa.com>
 * Copyright (C) Carl Klemm 2021 <carl@uvos.xyz>
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
#include <hildon/hildon-pannable-area.h>
#include <hildon/hildon-picker-button.h>
#include <hildon/hildon-stackable-window.h>
#endif

#include "comm.h"
#include "types.h"
#include "datapipe.h"
#include "datapipes.h"
#include "sphone-log.h"
#include "string.h"
#include "gtk-gui-utils.h"
#include "gui.h"
#include "sphone-modules.h"

/** Module name */
#define MODULE_NAME		"ui-sms-gtk"

/** Functionality provided by this module */
static const gchar *const provides[] = { MODULE_NAME, NULL };

/** Module information */
G_MODULE_EXPORT module_info_struct module_info = {
	/** Name of the module */
	.name = MODULE_NAME,
	/** Module provides */
	.provides = provides,
	/** Module priority */
	.priority = 250
};

static int gui_id;

static void gui_sms_send_callback(GtkWidget *button, GtkWidget *main_window);
static void gui_sms_cancel_callback(GtkWidget *button, GtkWidget *main_window);
static void gui_sms_reply_callback(GtkWidget *button);
static bool gtk_gui_sms_send_show(const MessageProperties *msg);
static void gui_sms_receive_show(const MessageProperties *message);

static void gui_sms_incoming_callback(gconstpointer data, gpointer user_data)
{
	const MessageProperties *message = (const MessageProperties*)data;
	(void)user_data;
	
	Contact contact = {0};
	contact.line_identifier = message->line_identifier;
	contact.backend = message->backend;
	if(gui_contact_shown(&contact))
		return;

	gui_sms_receive_show(message);
}

static void gui_sms_open_contact_callback(GtkButton *button)
{
	gchar *dial = g_object_get_data(G_OBJECT(button), "dial");
	(void)dial;
	// TODO replace:
	//gui_contact_open_by_dial(dial);
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

static bool gtk_gui_sms_send_show(const MessageProperties *msg)
{
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
	GtkWidget *backend_combo;
	GtkWidget *selector = NULL;
	(void)selector;
	
#ifdef ENABLE_LIBHILDON
	GtkWidget *main_window = hildon_stackable_window_new();
	selector = gui_sms_create_selector();
	backend_combo = hildon_picker_button_new(HILDON_SIZE_AUTO, HILDON_BUTTON_ARRANGEMENT_HORIZONTAL);
	hildon_button_set_title (HILDON_BUTTON(backend_combo), "Backend");
	hildon_picker_button_set_selector(HILDON_PICKER_BUTTON(backend_combo),
	                                 HILDON_TOUCH_SELECTOR(selector));
	hildon_gtk_window_set_portrait_flags(GTK_WINDOW(main_window), HILDON_PORTRAIT_MODE_SUPPORT);
	g_object_set_data(G_OBJECT(main_window), "backend-combo-box", selector);
	hildon_touch_selector_set_active(HILDON_TOUCH_SELECTOR(selector), 0, msg->backend);
#else
	GtkWidget *main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	backend_combo = gui_sms_create_backend_combo();
	g_object_set_data(G_OBJECT(main_window), "backend-combo-box", backend_combo);
	gtk_combo_box_set_active(GTK_COMBO_BOX(backend_combo), msg->backend);
#endif

	gtk_window_set_title(GTK_WINDOW(main_window),"Send SMS");
	gtk_window_set_default_size(GTK_WINDOW(main_window),400,220);

	GtkTextBuffer *text_buffer=gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_edit));

	if(msg && msg->line_identifier)
		gtk_entry_set_text(GTK_ENTRY(to_entry), msg->line_identifier);
	if(msg && msg->text)
		gtk_text_buffer_set_text(text_buffer, msg->text, -1);
	
	gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(text_edit), GTK_TEXT_WINDOW_LEFT, 2);
	gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(text_edit), GTK_TEXT_WINDOW_RIGHT, 2);

	gtk_box_pack_start(GTK_BOX(to_bar), to_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(to_bar), to_entry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(to_bar), backend_combo, TRUE, TRUE, 0);
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
	
	g_signal_connect(G_OBJECT(send_button),"clicked", G_CALLBACK(gui_sms_send_callback), main_window);
	g_signal_connect(G_OBJECT(cancel_button),"clicked", G_CALLBACK(gui_sms_cancel_callback), main_window);
	return true;
}

static void gui_sms_receive_show(const MessageProperties *message)
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
	
	time_str = gtk_gui_date_to_new_string(message->time);

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

	MessageProperties *message_copy = message_properties_copy(message);
	
	sphone_module_log(LL_DEBUG, "%s %p", __func__, message_copy);

	g_object_set_data_full(G_OBJECT(send_button), "message-proparties",
						   message_copy, (void (*)(void *))message_properties_free);
	
	g_signal_connect(G_OBJECT(from_entry),"clicked", G_CALLBACK(gui_sms_open_contact_callback),NULL);
	g_signal_connect(G_OBJECT(send_button),"clicked", G_CALLBACK(gui_sms_reply_callback),NULL);
	g_signal_connect(G_OBJECT(cancel_button),"clicked", G_CALLBACK(gui_sms_cancel_callback),main_window);
	
	g_free(time_str);
	g_free(desc);
}

static void gui_sms_send_callback(GtkWidget *button, GtkWidget *main_window)
{
	(void)button;
	GtkEntry *to_entry=g_object_get_data(G_OBJECT(main_window),"to_entry");
	GtkTextBuffer *text_buffer=g_object_get_data(G_OBJECT(main_window),"text_buffer");

	const gchar *to = gtk_entry_get_text(to_entry);
	gchar *text = NULL;
	g_object_get(G_OBJECT(text_buffer),"text", &text, NULL);
	
	char *backend_name = NULL;
	GtkWidget *backend_combo = g_object_get_data(G_OBJECT(main_window), "backend-combo-box");
	
#ifdef ENABLE_LIBHILDON
	backend_name = hildon_touch_selector_get_current_text(HILDON_TOUCH_SELECTOR(backend_combo));
#else
	backend_name = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(backend_combo));
#endif

	if(text && strlen(text) > 0 && strlen(to) > 0) {
		MessageProperties *message = g_malloc0(sizeof(*message));
		message->line_identifier = g_strdup(to);
		message->text = text;
		message->backend = sphone_comm_find_backend_id(backend_name);
		message->time = time(NULL);
		message->outbound = true;
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

static void gui_sms_cancel_callback(GtkWidget *button, GtkWidget *main_window)
{
	(void)button;
	gtk_widget_destroy(main_window);
}

static void gui_sms_reply_callback(GtkWidget *button)
{
	MessageProperties *msg = g_object_get_data(G_OBJECT(button), "message-proparties");
	gtk_gui_sms_send_show(msg);
}

G_MODULE_EXPORT const gchar *sphone_module_init(void** data);
const gchar *sphone_module_init(void** data)
{
	(void)data;
	append_trigger_to_datapipe(&message_recived_pipe, gui_sms_incoming_callback, NULL);
	gui_id = gui_register(NULL, gtk_gui_sms_send_show, NULL, NULL, NULL);
	return NULL;
}

G_MODULE_EXPORT void sphone_module_exit(void* data);
void sphone_module_exit(void* data)
{
	(void)data;
	gui_remove(gui_id);
	remove_trigger_from_datapipe(&message_recived_pipe, gui_sms_incoming_callback, NULL);
}
