/*
 * contacts-ui-exec.c
 * Copyright (C) Carl Philipp Klemm 2021 <carl@uvos.xyz>
 * 
 * contacts-ui-exec.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * contacts-ui-exec.c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib.h>
#include <libosso-abook/osso-abook.h>
#include <gtk/gtk.h>
#include <libebook-contacts/libebook-contacts.h>
#include "sphone-modules.h"
#include "sphone-log.h"
#include "sphone-conf.h"
#include "datapipes.h"
#include "datapipe.h"
#include "types.h"
#include "gui.h"
#include "comm.h"
#include "sphone-log.h"

/** Module name */
#define MODULE_NAME		"contacts-ui-abook"

/** Functionality provided by this module */
static const gchar *const provides[] = { "contacts-ui", NULL };

struct UiAbookPriv
{
	EBook* ebook;
	OssoABookRoster *roster;
	GtkWidget *chooser;
	GtkWidget *card;
	void (*callback)(Contact*, void*);
	void *user_data;
	int ui_id;
} abook_priv;

/** Module information */
SPHONE_MODULE_EXPORT module_info_struct module_info = {
	/** Name of the module */
	.name = MODULE_NAME,
	/** Module provides */
	.provides = provides,
	/** Module priority */
	.priority = 10
};

static GList *find_abook_contacts(const char *line_id, int id)
{
	(void)id;

	if(osso_abook_aggregator_get_state(OSSO_ABOOK_AGGREGATOR(abook_priv.roster)) != OSSO_ABOOK_AGGREGATOR_READY) {
		sphone_module_log(LL_WARN, "Abook is not ready %i", osso_abook_aggregator_get_state(OSSO_ABOOK_AGGREGATOR(abook_priv.roster)));
		//return NULL;
	}
	else {
		sphone_module_log(LL_DEBUG, "Abook is ready");
	}

	EPhoneNumber *enumber = e_phone_number_from_string(line_id, NULL, NULL);

	if(enumber) {
		gchar *number = e_phone_number_to_string(enumber, E_PHONE_NUMBER_FORMAT_E164);
		EBookQuery *query = e_book_query_orv(
			e_book_query_field_test(E_CONTACT_PHONE_ASSISTANT, E_BOOK_QUERY_EQUALS_SHORT_PHONE_NUMBER, number),
			e_book_query_field_test(E_CONTACT_PHONE_BUSINESS, E_BOOK_QUERY_EQUALS_SHORT_PHONE_NUMBER, number),
			e_book_query_field_test(E_CONTACT_PHONE_BUSINESS_2, E_BOOK_QUERY_EQUALS_SHORT_PHONE_NUMBER, number),
			e_book_query_field_test(E_CONTACT_PHONE_BUSINESS_FAX, E_BOOK_QUERY_EQUALS_SHORT_PHONE_NUMBER, number),
			e_book_query_field_test(E_CONTACT_PHONE_CALLBACK, E_BOOK_QUERY_EQUALS_SHORT_PHONE_NUMBER, number),
			e_book_query_field_test(E_CONTACT_PHONE_CAR, E_BOOK_QUERY_EQUALS_SHORT_PHONE_NUMBER, number),
			e_book_query_field_test(E_CONTACT_PHONE_COMPANY, E_BOOK_QUERY_EQUALS_SHORT_PHONE_NUMBER, number),
			e_book_query_field_test(E_CONTACT_PHONE_HOME, E_BOOK_QUERY_EQUALS_SHORT_PHONE_NUMBER, number),
			e_book_query_field_test(E_CONTACT_PHONE_HOME_2, E_BOOK_QUERY_EQUALS_SHORT_PHONE_NUMBER, number),
			e_book_query_field_test(E_CONTACT_PHONE_HOME_FAX, E_BOOK_QUERY_EQUALS_SHORT_PHONE_NUMBER, number),
			e_book_query_field_test(E_CONTACT_PHONE_ISDN, E_BOOK_QUERY_EQUALS_SHORT_PHONE_NUMBER, number),
			e_book_query_field_test(E_CONTACT_PHONE_MOBILE, E_BOOK_QUERY_EQUALS_SHORT_PHONE_NUMBER, number),
			e_book_query_field_test(E_CONTACT_PHONE_OTHER, E_BOOK_QUERY_EQUALS_SHORT_PHONE_NUMBER, number),
			e_book_query_field_test(E_CONTACT_PHONE_OTHER_FAX, E_BOOK_QUERY_EQUALS_SHORT_PHONE_NUMBER, number),
			e_book_query_field_test(E_CONTACT_PHONE_PAGER, E_BOOK_QUERY_EQUALS_SHORT_PHONE_NUMBER, number),
			e_book_query_field_test(E_CONTACT_PHONE_PRIMARY, E_BOOK_QUERY_EQUALS_SHORT_PHONE_NUMBER, number),
			e_book_query_field_test(E_CONTACT_PHONE_RADIO, E_BOOK_QUERY_EQUALS_SHORT_PHONE_NUMBER, number),
			e_book_query_field_test(E_CONTACT_PHONE_TELEX, E_BOOK_QUERY_EQUALS_SHORT_PHONE_NUMBER, number),
			e_book_query_field_test(E_CONTACT_PHONE_TTYTDD, E_BOOK_QUERY_EQUALS_SHORT_PHONE_NUMBER, number),
			NULL);

		GList *contacts = osso_abook_aggregator_find_contacts(OSSO_ABOOK_AGGREGATOR(abook_priv.roster), query);
		e_book_query_unref(query);
		return contacts;
	}

	return NULL;
}

static void contact_dialog_reponse_cb(GtkDialog *dialog, int response_id, void *data)
{
	(void)data;
	(void)dialog;
	(void)response_id;

	GList *selection =
		osso_abook_contact_chooser_get_selection(OSSO_ABOOK_CONTACT_CHOOSER(abook_priv.chooser));
		
	gtk_widget_destroy(abook_priv.chooser);
	abook_priv.chooser = NULL;
		
	if (selection) {
		OssoABookContact *acontact = selection->data;
		OssoABookContactDetailSelector *detailDiag = 
		(OssoABookContactDetailSelector*)osso_abook_contact_detail_selector_new_for_contact(
			NULL, acontact, OSSO_ABOOK_CONTACT_DETAIL_PHONE | OSSO_ABOOK_CONTACT_DETAIL_IM_VOICE);
		gtk_dialog_run(GTK_DIALOG(detailDiag));
		
		EVCardAttribute *attribute = osso_abook_contact_detail_selector_get_detail(detailDiag);
		
		if(!attribute) {
			abook_priv.callback = NULL;
			return;
		}

		{
			Contact contact = {};

			contact.name = (char*)osso_abook_contact_get_display_name(acontact);
			//callContact.photo = osso_abook_contact_get_photo(acontact);
			//g_object_ref(G_OBJECT(callContact.photo));
			contact.line_identifier = e_vcard_attribute_get_value(attribute);

			if(abook_priv.callback)
				abook_priv.callback(&contact, abook_priv.user_data);

			g_list_free(selection);
			gtk_widget_destroy(GTK_WIDGET(detailDiag));
		}
	}

	abook_priv.callback = NULL;
}

static void abook_dialog_close(void)
{
	if (abook_priv.chooser) {
		gtk_widget_destroy(abook_priv.chooser);
		abook_priv.chooser = NULL;
	}
	if(abook_priv.card) {
		gtk_widget_destroy(abook_priv.card);
		abook_priv.card = NULL;
	}
	abook_priv.callback = NULL;
}

static void abook_contact_show(const Contact *contact, void (*callback)(Contact*, void*), void *user_data)
{
	if(abook_priv.callback || abook_priv.card || abook_priv.chooser) {
		sphone_module_log(LL_WARN, "Only one dialog can be shown at a time");
		return;
	}

	if(contact) {
		if(abook_priv.card) {
			gtk_widget_destroy(abook_priv.card);
			abook_priv.card = NULL;
			abook_priv.callback = NULL;
		}
		if(!contact->line_identifier) {
			sphone_module_log(LL_WARN, "Can not display card for contact with no line_identifier");
			return;
		}
		GList *contacts = find_abook_contacts(contact->line_identifier, contact->backend);
		if(!contacts) {
			sphone_module_log(LL_INFO, "Could not find abook contact for %s", contact->line_identifier);
			CallProperties msg = {0};
			msg.backend = contact->backend;
			msg.line_identifier = contact->line_identifier;
			gui_dialer_show(&msg);
			return;
		}

		abook_priv.card = osso_abook_contact_detail_selector_new_for_contact(NULL, contacts->data, OSSO_ABOOK_CONTACT_DETAIL_ALL);
		g_list_free(contacts);
	}
	else {
		abook_priv.callback = callback;
		abook_priv.user_data = user_data;

		if(!abook_priv.chooser) {
			abook_priv.chooser = osso_abook_contact_chooser_new_with_capabilities(NULL, "Choose contact",
																			OSSO_ABOOK_CAPS_PHONE |
																			OSSO_ABOOK_CAPS_VOICE,
																			OSSO_ABOOK_CONTACT_ORDER_NAME);
			osso_abook_contact_chooser_set_maximum_selection(OSSO_ABOOK_CONTACT_CHOOSER(abook_priv.chooser), 1);
			osso_abook_contact_chooser_set_minimum_selection(OSSO_ABOOK_CONTACT_CHOOSER(abook_priv.chooser), 1);
			g_signal_connect(G_OBJECT(abook_priv.chooser), "response", G_CALLBACK(contact_dialog_reponse_cb), NULL);
		}

		gtk_widget_show_all(abook_priv.chooser);
	}
}

SPHONE_MODULE_EXPORT const gchar *sphone_module_init(void** data);
const gchar *sphone_module_init(void** data)
{
	(void)data;

	ESourceRegistry *regestry = e_source_registry_new_sync(NULL,NULL);
	ESource *address_book_src;
	gchar *source_uid = sphone_conf_get_string("ContactsEvolution", "ContactsSource", NULL, NULL);
	if(source_uid) {
		address_book_src = e_source_registry_ref_source(regestry, source_uid);
		g_free(source_uid);
	} else {
		address_book_src = e_source_registry_ref_default_address_book(regestry);
	}
	g_object_unref(regestry);

	if(!address_book_src) {
		sphone_module_log(LL_ERR, "Can not register for ebook source, can not continue");
		return "Can not register for ebook source";
	}

	GError* err = NULL;
	abook_priv.ebook = e_book_new(address_book_src, &err);
	g_object_unref(address_book_src);
	if(err) {
		sphone_module_log(LL_ERR, "Can not create ebook: %s", err->message);
		g_error_free(err);
		return "Can not create ebook";
	}

	e_book_open(abook_priv.ebook, TRUE, &err);
	if(err) {
		sphone_module_log(LL_ERR, "Can not open ebook: %s", err->message);
		g_error_free(err);
		return "Can not open ebook";
	}

	abook_priv.roster = osso_abook_aggregator_new(abook_priv.ebook, &err);
	if(!abook_priv.roster) {
		sphone_module_log(LL_WARN, "Could not get abook aggregator: %s", err->message);
		g_error_free(err);
	}

	hildon_init();
	osso_abook_init_with_name("sphone", NULL);

	struct GuiFunctions func = {};
	func.contact_show = abook_contact_show;
	func.close_contact_diag = abook_dialog_close;
	abook_priv.ui_id = gui_register(func);

	return NULL;
}

SPHONE_MODULE_EXPORT void sphone_module_exit(void* data);
void sphone_module_exit(void* data)
{
	(void)data;
	gui_remove(abook_priv.ui_id);
	abook_dialog_close();
	g_object_unref(abook_priv.roster);
	g_object_unref(abook_priv.ebook);
	if(abook_priv.chooser)
		gtk_widget_destroy(abook_priv.chooser);
	if(abook_priv.card)
		gtk_widget_destroy(abook_priv.card);

}
