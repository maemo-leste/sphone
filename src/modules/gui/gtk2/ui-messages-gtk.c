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

#include <glib.h>
#include <gtk/gtk.h>
#include <time.h>

#ifdef ENABLE_LIBHILDON
#include <hildon/hildon.h>
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
static bool gtk_gui_sms_send_show(const MessageProperties *msg);

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

static void gui_sms_contact_show_callback(Contact* contact, void* user_data)
{
	if(contact)
		gtk_entry_set_text(GTK_ENTRY(user_data), contact->line_identifier);
}

static void gui_sms_contacts_callback(GtkWidget *button, GtkWidget *to_entry)
{
	(void)button;
	gui_contact_show(NULL, gui_sms_contact_show_callback, to_entry);
}

static int gui_sms_send_close(GtkWidget *w)
{
	(void)w;
	gui_close_contact_diag();

	return FALSE;
}

static void gui_sms_cancel_callback(GtkWidget *button, GtkWidget *main_window)
{
	(void)button;
	gtk_widget_destroy(main_window);
}

static bool gtk_gui_sms_send_show(const MessageProperties *msg)
{
	GtkWidget *v1 = gtk_vbox_new(FALSE,2);
	GtkWidget *to_bar = gtk_hbox_new(FALSE,0);
	GtkWidget *actions_bar = gtk_hbox_new(FALSE,0);
	GtkWidget *to_label = gtk_label_new("To:");
	GtkWidget *to_entry = gtk_entry_new();
	GtkWidget *send_button = gtk_button_new_with_label("Send");
	GtkWidget *cancel_button = gtk_button_new_with_label("Cancel");
	GtkWidget *contacts_button = gtk_button_new_with_label("Contacts");
	GtkWidget *text_edit = gtk_text_view_new();
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
	if(msg && msg->backend > 0)
		hildon_touch_selector_set_active(HILDON_TOUCH_SELECTOR(selector), 0, msg->backend);
#else
	GtkWidget *main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	backend_combo = gui_sms_create_backend_combo();
	g_object_set_data(G_OBJECT(main_window), "backend-combo-box", backend_combo);
	if(msg && msg->backend > 0)
		gtk_combo_box_set_active(GTK_COMBO_BOX(backend_combo), msg->backend);
#endif

	gtk_window_set_title(GTK_WINDOW(main_window),"Send SMS");
	gtk_window_set_default_size(GTK_WINDOW(main_window),400,220);

	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_edit), GTK_WRAP_WORD_CHAR);
	GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_edit));

	if(msg && msg->line_identifier)
		gtk_entry_set_text(GTK_ENTRY(to_entry), msg->line_identifier);
	if(msg && msg->text)
		gtk_text_buffer_set_text(text_buffer, msg->text, -1);
	
	gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(text_edit), GTK_TEXT_WINDOW_LEFT, 2);
	gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(text_edit), GTK_TEXT_WINDOW_RIGHT, 2);

	gtk_box_pack_start(GTK_BOX(to_bar), to_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(to_bar), to_entry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(to_bar), contacts_button, TRUE, TRUE, 0);
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
	g_signal_connect(G_OBJECT(contacts_button),"clicked", G_CALLBACK(gui_sms_contacts_callback), to_entry);
	g_signal_connect(G_OBJECT(cancel_button),"clicked", G_CALLBACK(gui_sms_cancel_callback), main_window);
	g_signal_connect(G_OBJECT(main_window), "delete-event", G_CALLBACK(gui_sms_send_close),NULL);
	return true;
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

G_MODULE_EXPORT const gchar *sphone_module_init(void** data);
const gchar *sphone_module_init(void** data)
{
	(void)data;
#ifdef ENABLE_LIBHILDON
	hildon_init();
#endif
	gui_id = gui_register(NULL, gtk_gui_sms_send_show, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	return NULL;
}

G_MODULE_EXPORT void sphone_module_exit(void* data);
void sphone_module_exit(void* data)
{
	(void)data;
	gui_remove(gui_id);
}
