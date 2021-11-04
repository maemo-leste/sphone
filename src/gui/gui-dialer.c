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

#include <ctype.h>
#include <gtk/gtk.h>

#ifdef ENABLE_LIBHILDON
#include <hildon/hildon-gtk.h>
#include <hildon/hildon-pannable-area.h>
#include <hildon/hildon-picker-button.h>
#endif

#include "datapipes.h"
#include "datapipe.h"
#include "comm.h"
#include "types.h"
#include "sphone-log.h"
#include "keypad.h"
#include "gui-dialer.h"
#include "sphone-store-tree-model.h"
#include "gui-history-calls.h"

struct{
	GtkWidget *display;
	GtkWidget *main_window;
	GtkWidget *dials_view;
	GtkWidget *book;
	GtkWidget *backend_combo;
	GtkWidget *selector;
}g_gui_calls;

#ifdef ENABLE_LIBHILDON
static GtkWidget *gui_dialer_create_selector(void)
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

static GtkWidget *gui_dialer_create_backend_combo(void)
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

static void gui_call_callback(GtkButton button)
{
	(void)button;

	const gchar *dial = gtk_entry_get_text(GTK_ENTRY(g_gui_calls.display));
	char *backend_name = NULL;
	
#ifdef ENABLE_LIBHILDON
	backend_name = hildon_touch_selector_get_current_text(HILDON_TOUCH_SELECTOR(g_gui_calls.selector));
#else
	backend_name = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(g_gui_calls.backend_combo));
#endif

	if(strlen(dial) > 0 && backend_name) {
		CallProperties *call = g_malloc0(sizeof(*call));
		call->line_identifier = g_strdup(dial);
		call->backend = sphone_comm_find_backend_id(backend_name);
		call->state = SPHONE_CALL_DIALING;
		execute_datapipe(&call_dial_pipe, call);
		call_properties_free(call);

		gtk_entry_set_text(GTK_ENTRY(g_gui_calls.display), "");
		gtk_widget_hide(g_gui_calls.main_window);
	}
}

static void gui_dialer_cancel_callback(void)
{
	gtk_entry_set_text(GTK_ENTRY(g_gui_calls.display), "");
	gtk_widget_hide(g_gui_calls.main_window);
}

static void gui_dialer_back_presses_callback(GtkWidget *button, GtkWidget *target)
{
	(void)button;
	gtk_editable_set_position(GTK_EDITABLE(target),-1);
	gint position=gtk_editable_get_position(GTK_EDITABLE(target));
	gtk_editable_delete_text (GTK_EDITABLE(target),position-1, position);

	gtk_editable_set_position(GTK_EDITABLE(target),position);
}

static void gui_contacts_callback(void)
{
	sphone_log(LL_DEBUG, "%s", __func__);
	execute_datapipe(&contact_show_pipe, NULL);
}

static void gui_history_show(void)
{
	sphone_log(LL_DEBUG, "%s", __func__);
	gui_history_calls();
}

static void gui_dialer_validate_callback(GtkEntry *entry,const gchar *text, gint length, gint *position,gpointer data)
{
	int count = 0;
	gchar *result = g_new(gchar, length+1);

	for (int i = 0; i < length && text[i]; ++i) {
		if (!isdigit(text[i]) && text[i] != '*' && text[i] != '#'  && text[i] != '+')
			continue;
		result[count++] = text[i];
	}
	result[count] = '\0';

	if (count > 0) {
		GtkEditable *editable  = GTK_EDITABLE(entry);
		g_signal_handlers_block_by_func (G_OBJECT (editable),	G_CALLBACK (gui_dialer_validate_callback),data);
		gtk_editable_insert_text (editable, result, count, position);
		g_signal_handlers_unblock_by_func (G_OBJECT (editable),	G_CALLBACK (gui_dialer_validate_callback),data);
	}
	g_signal_stop_emission_by_name (G_OBJECT(entry), "insert_text");

	g_free (result);
}

void gtk_gui_dialer_init(void)
{
	g_gui_calls.main_window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(g_gui_calls.main_window),"Dialer");
	gtk_window_set_deletable(GTK_WINDOW(g_gui_calls.main_window),FALSE);
	gtk_window_set_default_size(GTK_WINDOW(g_gui_calls.main_window),400,220);
	gtk_window_maximize(GTK_WINDOW(g_gui_calls.main_window));
	GtkWidget *v1 = gtk_vbox_new(FALSE, 5);
	GtkWidget *actions_bar = gtk_hbox_new(TRUE,0);
	GtkWidget *contacts_bar = gtk_hbox_new(TRUE,0);
	GtkWidget *display_back = gtk_button_new_with_label ("\n    <    \n");
	GtkWidget *display = gtk_entry_new();
	GtkWidget *display_bar = gtk_hbox_new(FALSE,4);
	GtkWidget *keypad = gui_keypad_setup(display);
	GtkWidget *call_button = gtk_button_new_with_label("\nCall\n");
	GtkWidget *cancel_button = gtk_button_new_with_label("\nCancel\n");
	GtkWidget *contacts_button = gtk_button_new_with_label("\nContacts\n");
	GtkWidget *recents_button = gtk_button_new_with_label("\nRecent\n");
	
#ifdef ENABLE_LIBHILDON
	g_gui_calls.selector = gui_dialer_create_selector();
	g_gui_calls.backend_combo = hildon_picker_button_new (HILDON_SIZE_AUTO, HILDON_BUTTON_ARRANGEMENT_HORIZONTAL);
	hildon_button_set_title (HILDON_BUTTON(g_gui_calls.backend_combo), "\nBackend\n");
	hildon_picker_button_set_selector(HILDON_PICKER_BUTTON(g_gui_calls.backend_combo),
	                                 HILDON_TOUCH_SELECTOR(g_gui_calls.selector));
	hildon_gtk_window_set_portrait_flags(GTK_WINDOW(g_gui_calls.main_window), HILDON_PORTRAIT_MODE_REQUEST);
#else
	g_gui_calls.backend_combo = gui_dialer_create_backend_combo();
#endif
	
	GdkColor white, black;
	GtkWidget *e = gtk_event_box_new ();
	g_gui_calls.display=display;
	
	gtk_widget_modify_font(display, pango_font_description_from_string("Monospace 24"));
	gtk_entry_set_alignment(GTK_ENTRY(display),1.0);
	gtk_entry_set_has_frame(GTK_ENTRY(display),FALSE);
		
	gdk_color_parse("black", &black);
	gdk_color_parse("white", &white);
	
	gtk_widget_modify_bg(e,GTK_STATE_NORMAL,&black);
	
	gtk_container_add(GTK_CONTAINER(e), display);
	gtk_box_pack_start(GTK_BOX(display_bar), e, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(display_bar), display_back, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(v1),display_bar,FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(contacts_bar), contacts_button);
	gtk_container_add(GTK_CONTAINER(contacts_bar), recents_button);
	gtk_box_pack_start(GTK_BOX(v1), contacts_bar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(v1), g_gui_calls.backend_combo, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(v1), keypad, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(actions_bar), call_button);
	gtk_container_add(GTK_CONTAINER(actions_bar), cancel_button);
	gtk_box_pack_start(GTK_BOX(v1), actions_bar, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(g_gui_calls.main_window), v1);

	gtk_widget_show_all(v1);
	gtk_widget_grab_focus(display);
	
	g_signal_connect(G_OBJECT(call_button),"clicked", G_CALLBACK(gui_call_callback),NULL);
	g_signal_connect(G_OBJECT(cancel_button),"clicked", G_CALLBACK(gui_dialer_cancel_callback),NULL);
	g_signal_connect(G_OBJECT(contacts_button),"clicked", G_CALLBACK(gui_contacts_callback),NULL);
	g_signal_connect(G_OBJECT(recents_button),"clicked", G_CALLBACK(gui_history_show),NULL);
	g_signal_connect(G_OBJECT(g_gui_calls.main_window),"delete-event", G_CALLBACK(gtk_widget_hide_on_delete),NULL);
	g_signal_connect(G_OBJECT(display_back), "clicked", G_CALLBACK(gui_dialer_back_presses_callback), display);
	g_signal_connect(G_OBJECT(display), "insert_text", G_CALLBACK(gui_dialer_validate_callback),NULL);
}

bool gtk_gui_dialer_show(const CallProperties* call)
{
	gtk_window_present(GTK_WINDOW(g_gui_calls.main_window));
	gtk_widget_grab_focus(g_gui_calls.display);

	if(call && call->line_identifier) {
		sphone_log(LL_DEBUG, "%s: with %s", __func__, call->line_identifier);
		gtk_entry_set_text(GTK_ENTRY(g_gui_calls.display), call->line_identifier);
	}
	return true;
}
