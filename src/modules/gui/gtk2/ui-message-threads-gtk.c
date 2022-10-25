/*
 * ui-message-threads-gtk.c
 * Copyright (C) Carl Philipp Klemm 2021 <carl@uvos.xyz>
 * 
 * ui-message-threads-gtk.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * ui-message-threads-gtk.c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef ENABLE_LIBHILDON
#include <hildon/hildon.h>
#include <hildon/hildon-gtk.h>
#include <hildon/hildon-pannable-area.h>
#include <hildon/hildon-stackable-window.h>
#include <hildon/hildon-text-view.h>
#endif

#include <gtk/gtk.h>

#include "sphone-log.h"
#include "gui.h"
#include "comm.h"
#include "storage.h"
#include "gtk-gui-utils.h"
#include "datapipes.h"
#include "datapipe.h"
#include "sphone-modules.h"
#include "gtk-gui-message-threads.h"

/** Module name */
#define MODULE_NAME		"ui-message-threads-gtk"

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

static GSList *shown_contacts;

static int gui_id;

static bool gtk_gui_contact_shown(const Contact *contact)
{
	for(GSList *element = shown_contacts; element; element = element->next) {
		const Contact *icontact = element->data;
		if(contact_cmp(contact, icontact))
			return true;
	}
	return false;
}

static void new_message_trigger(const void* data, void *user_data)
{
	GtkTextView *text_view = GTK_TEXT_VIEW(user_data);
	GtkTextBuffer *text;
	
#ifdef ENABLE_LIBHILDON
	text = hildon_text_view_get_buffer(HILDON_TEXT_VIEW(text_view));
#else
	text = gtk_text_view_get_buffer(text_view);
#endif
	
	const MessageProperties *msg = data;
	const Contact *watch_contact = g_object_get_data(G_OBJECT(text), "contact");
	if(watch_contact->backend == msg->backend &&
		g_strcmp0(watch_contact->line_identifier, msg->line_identifier) == 0) {
		GString *string = g_string_new(NULL);
		char *time = gtk_gui_time_to_new_string(msg->time);
		const char *name;
		if(!msg->outbound) {
			name = msg->contact && msg->contact->name ? msg->contact->name : msg->line_identifier;
		} else {
			name = getenv("LOGNAME") ?: "sphone";
		}
		g_string_append_printf(string, "\n[%s] <%s> %s", time, name, msg->text);
		GtkTextIter iter;
		gtk_text_buffer_get_end_iter(text, &iter);
		gtk_text_buffer_insert(text, &iter, string->str, string->len);
		gtk_text_buffer_get_end_iter(text, &iter);
		//gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(text_view), &iter,  0, FALSE, 0, 0);
		g_free(time);
		g_string_free(string, TRUE);
	}
}

static void remove_thread_view(GtkWidget *widget, gpointer data)
{
	(void)widget;
	GtkWidget *text_view = GTK_WIDGET(data);
	const Contact *watch_contact = g_object_get_data(G_OBJECT(text_view), "contact");
	shown_contacts = g_slist_remove(shown_contacts, watch_contact);
	remove_trigger_from_datapipe(&message_send_pipe, new_message_trigger, text_view);
	remove_trigger_from_datapipe(&message_received_pipe, new_message_trigger, text_view);
}

static void gtk_gui_thread_view_reply_cb(GtkButton* button, Contact *contact)
{
	(void)button;
	MessageProperties msg = {0};
	msg.contact = contact;
	msg.line_identifier = contact->line_identifier;
	msg.backend = contact->backend;
	contact_print(contact, __func__);
	gui_sms_send_show(&msg);
}

static GtkTextBuffer *gtk_gui_build_text_buffer(GList *msg_list)
{
	GString *string = g_string_new(NULL);
	GList *last_element = g_list_last(msg_list);
	for(GList *element = last_element; element; element = element->prev) {
		MessageProperties *msg = element->data;
		char *time = gtk_gui_time_to_new_string(msg->time);
		const char *name;
		if(!msg->outbound) {
			name = msg->contact && msg->contact->name ? msg->contact->name : msg->line_identifier;
		} else {
			name = getenv("LOGNAME") ?: "sphone";
		}
		g_string_append_printf(string, "%s[%s] <%s> %s", element != g_list_last(msg_list) ? "\n" : "", time, name, msg->text);
		g_free(time);
	}
	GtkTextBuffer *text = gtk_text_buffer_new(NULL);
	gtk_text_buffer_set_text(text, string->str, string->len);
	g_string_free(string, TRUE);
	return text;
}

static void gtk_gui_show_thread_for_contact(const Contact *contact)
{
	sphone_log(LL_DEBUG, "gtk_gui_thread_calls\n");

	GtkWidget *v1 = gtk_vbox_new(FALSE, 0);
	GtkWidget *text_view;
	GtkWidget *text_view_scroll;
	GtkWidget *reply_button = gtk_button_new_with_label("Reply");
	GtkWidget *actions_bar = gtk_hbox_new(TRUE,0);
	GtkTextMark *text_mark_end;
	
#ifdef ENABLE_LIBHILDON
	GtkWidget *window = hildon_stackable_window_new();
	text_view = hildon_text_view_new();
	hildon_gtk_window_set_portrait_flags(GTK_WINDOW(window), HILDON_PORTRAIT_MODE_SUPPORT);
	text_view_scroll = hildon_pannable_area_new();
#else
	GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	text_view = gtk_text_view_new();
	text_view_scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(text_view_scroll), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
#endif
	
	sphone_log(LL_DEBUG, "v1 FLOATING: %i", g_object_is_floating(v1));
	sphone_log(LL_DEBUG, "window FLOATING: %i", g_object_is_floating(window));
	sphone_log(LL_DEBUG, "text_view FLOATING: %i", g_object_is_floating(text_view));

	gtk_window_set_title(GTK_WINDOW(window), contact->name ?: "Thread");
	gtk_window_set_default_size(GTK_WINDOW(window), 400, 600);

	gtk_container_add(GTK_CONTAINER(text_view_scroll), text_view);
	gtk_container_add(GTK_CONTAINER(actions_bar), reply_button);
	gtk_container_add(GTK_CONTAINER(v1), text_view_scroll);
	gtk_box_pack_start(GTK_BOX(v1), actions_bar, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window), v1);

	Contact *contact_cpy = contact_copy(contact);
	GList *msg_list = store_get_messages_for_contact(contact_cpy);
	GtkTextBuffer *text = gtk_gui_build_text_buffer(msg_list);
	g_object_set_data_full(G_OBJECT(text), "contact", contact_cpy, (GDestroyNotify)contact_free);
	g_signal_connect(GTK_WIDGET(window), "hide", G_CALLBACK(remove_thread_view), text_view);
	shown_contacts = g_slist_prepend(shown_contacts, contact_cpy);
	append_trigger_to_datapipe(&message_send_pipe, new_message_trigger, text_view);
	append_trigger_to_datapipe(&message_received_pipe, new_message_trigger, text_view);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), false);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view), false);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD_CHAR);

#ifdef ENABLE_LIBHILDON
	hildon_text_view_set_buffer(HILDON_TEXT_VIEW(text_view), text);
#else
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(text_view), text);
#endif

	g_object_unref(text);
	store_free_message_list(msg_list);
	g_signal_connect(G_OBJECT(reply_button), "clicked", G_CALLBACK(gtk_gui_thread_view_reply_cb), contact_cpy);

	gtk_widget_show_all(window);
	
	GtkTextIter iter;
	gtk_text_buffer_get_end_iter(text, &iter);
	text_mark_end = gtk_text_buffer_create_mark(text, "end", &iter, TRUE);
	gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(text_view), text_mark_end, 0., FALSE, 0., 0.);
}

G_MODULE_EXPORT const gchar *sphone_module_init(void** data);
const gchar *sphone_module_init(void** data)
{
	(void)data;
#ifdef ENABLE_LIBHILDON
	hildon_init();
#endif
	gui_id = gui_register(NULL, NULL, NULL, gtk_gui_msg_threads, gtk_gui_contact_shown, gtk_gui_show_thread_for_contact, NULL, NULL, NULL);
	return NULL;
}

G_MODULE_EXPORT void sphone_module_exit(void* data);
void sphone_module_exit(void* data)
{
	(void)data;
	gui_remove(gui_id);
}
