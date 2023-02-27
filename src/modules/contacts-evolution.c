/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */

#include <libebook/libebook.h>
#include <libebook-contacts/libebook-contacts.h>
#include <glib.h>
#include "sphone-modules.h"
#include "sphone-log.h"
#include "sphone-conf.h"
#include "datapipe.h"
#include "datapipes.h"
#include "types.h"
#include "comm.h"

/** Module name */
#define MODULE_NAME		"contacts-evolution"

/** Functionality provided by this module */
static const gchar *const provides[] = { MODULE_NAME, NULL };

/** Module information */
G_MODULE_EXPORT module_info_struct module_info = {
	/** Name of the module */
	.name = MODULE_NAME,
	/** Module provides */
	.provides = provides,
	/** Module priority */
	.priority = 10
};

struct evolution_priv {
	EBookClient *ebook;
};

static GSList *find_e_contacts(EBookClient *ebook, const char *line_id, int id)
{
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

		gchar *query_string = e_book_query_to_string(query);
		e_book_query_unref(query);

		g_free(number);
		e_phone_number_free(enumber);

		GError* error;

		GSList *contacts = NULL;

		if(!e_book_client_get_contacts_sync(ebook, query_string, &contacts, NULL, &error)) {
			g_free(query_string);
			sphone_module_log(LL_DEBUG, "e_book_client_get_contacts_sync failed %s", error ? error->message : "");
			return NULL;
		}

		return contacts;
	}

	return NULL;
}

static bool fill_contact(EBookClient *ebook, const char *line_id, Contact *contact, int id)
{
	GSList *contacts = find_e_contacts(ebook, line_id, id);

	if(!contacts)
		return false;

	EContact *econtact = contacts->data;
	contact->name = g_strdup(e_contact_get_const(econtact, E_CONTACT_FULL_NAME));
	e_client_util_free_object_slist(contacts);

	contact->line_identifier = g_strdup(line_id);
	contact->backend = id;

	return (bool)contact->name;
}

static gpointer call_filter(gpointer data, gpointer user_data)
{
	CallProperties *call = data;
	struct evolution_priv *priv = user_data;
	if(priv->ebook && !call->contact) {
		const CommBackend *backend = sphone_comm_get_backend(call->backend);
		if(backend && g_strcmp0(backend->name, "ofono") == 0) {
			call->contact = g_malloc0(sizeof(*call->contact));
			if(!fill_contact(priv->ebook, call->line_identifier, call->contact, call->backend)) {
				g_free(call->contact);
				call->contact = NULL;
			} else {
				sphone_module_log(LL_DEBUG, "got contact: %s", call->contact->name);
			}
		} else {
			sphone_module_log(LL_WARN, "can currently only fill contacts of ofono calls");
		}
	}
	return call;
}

static gpointer message_filter(gpointer data, gpointer user_data)
{
	MessageProperties *msg = data;
	struct evolution_priv *priv = user_data;
	if(priv->ebook && !msg->contact) {
		const CommBackend *backend = sphone_comm_get_backend(msg->backend);
		if(backend && g_strcmp0(backend->name, "ofono") == 0) {
			msg->contact = g_malloc0(sizeof(*msg->contact));
			if(!fill_contact(priv->ebook, msg->line_identifier, msg->contact, msg->backend)) {
				g_free(msg->contact);
				msg->contact = NULL;
			} else {
				sphone_module_log(LL_DEBUG, "got contact: %s", msg->contact->name);
			}
		} else {
			sphone_module_log(LL_WARN, "can currently only fill contacts of ofono calls");
		}
	}
	return msg;
}

static gpointer contact_filter(gpointer data, gpointer user_data)
{
	Contact *contact = data;
	struct evolution_priv *priv = user_data;
	if(priv->ebook && !contact->name) {
		const CommBackend *backend = sphone_comm_get_backend(contact->backend);
		if(backend && g_strcmp0(backend->name, "ofono") == 0) {
			gchar *line_id = contact->line_identifier;
			fill_contact(priv->ebook, line_id, contact, contact->backend);
			if(line_id != contact->line_identifier)
				g_free(line_id);
		} else {
			sphone_module_log(LL_WARN, "can currently only fill contacts of ofono calls");
		}
	}
	return contact;
}

static void book_ready_callback(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
	(void)source_object;
	struct evolution_priv *priv = user_data;
	GError *error = NULL;
	
	//WTF gnome people why dose the constructor for EBookClient gobject return a EBookClient* object casted to EClient*
	priv->ebook = E_BOOK_CLIENT(e_book_client_connect_finish(res, &error));

	if(error) {
		sphone_module_log(LL_ERR, "e_book_client_connect_finish failed with: %s", error->message);
		g_error_free(error);
	} else {
		sphone_module_log(LL_INFO, "Sucessfully connected to evolution");
	}
}

G_MODULE_EXPORT const gchar *sphone_module_init(void** data);
const gchar *sphone_module_init(void** data)
{
	struct evolution_priv *book = g_malloc0(sizeof(*book));
	*data = book;
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

	if(address_book_src) {
		e_book_client_connect(address_book_src, 0, NULL, book_ready_callback, book);
		append_filter_to_datapipe(&call_new_pipe, call_filter, book);
		append_filter_to_datapipe(&call_properties_changed_pipe, call_filter, book);
		append_filter_to_datapipe(&message_received_pipe, message_filter, book);
		append_filter_to_datapipe(&contact_fill_pipe, contact_filter, book);
		g_object_unref(address_book_src);
	}
	return NULL;
}

G_MODULE_EXPORT void sphone_module_exit(void* data);
void sphone_module_exit(void* data)
{
	remove_filter_from_datapipe(&call_new_pipe, call_filter, data);
	remove_filter_from_datapipe(&call_properties_changed_pipe, call_filter, data);
	remove_filter_from_datapipe(&message_received_pipe, message_filter, data);
	remove_filter_from_datapipe(&contact_fill_pipe, contact_filter, data);
}
