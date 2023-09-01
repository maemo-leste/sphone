/*
 * storage.c
 * Copyright (C) Carl Philipp Klemm 2021 <carl@uvos.xyz>
 * 
 * storage.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * storage.c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "storage.h"
#include "sphone-log.h"
#include "datapipes.h"
#include "datapipe.h"

GList *(*get_messages_for_contact_backend)(Contact *contact, unsigned int limit);
GList *(*get_calls_for_contact_backend)(Contact *contact, unsigned int limit);

int store_register_backend(GList *(*get_messages_for_contact)(Contact *contact, unsigned int limit),
						   GList *(*get_calls_for_contact)(Contact *contact, unsigned int limit))
{
	get_messages_for_contact_backend = get_messages_for_contact;
	get_calls_for_contact_backend = get_calls_for_contact;
	return 0;
}

GList *store_get_messages_for_contact(Contact *contact, unsigned int limit)
{
	if(!get_messages_for_contact_backend) {
		sphone_log(LL_ERR, "%s used without backend", __func__);
		return NULL;
	}
	return get_messages_for_contact_backend(contact, limit);
}

GList *store_get_calls_for_contact(Contact *contact, unsigned int limit)
{
	if(!get_calls_for_contact_backend) {
		sphone_log(LL_ERR, "%s used without backend", __func__);
		return NULL;
	}
	return get_calls_for_contact_backend(contact, limit);
}

static bool store_is_contact_in_list(GList *contacts, const char* line_id, int backend)
{
	for(GList *element = contacts; element; element = element->next) {
		Contact* contact = element->data;
		if(contact->backend == backend && g_strcmp0(contact->line_identifier, line_id) == 0)
			return true;
	}
	return false;
}

GList *store_get_interacted_msg_contacts(void)
{
	GList *messages = store_get_messages(0);
	GList *contacts = NULL;
	for(GList *element = messages; element; element = element->next) {
		MessageProperties* msg = element->data;
		if(!store_is_contact_in_list(contacts, msg->line_identifier, msg->backend)) {
			if(msg->contact && msg->contact->line_identifier) {
				if(!msg->contact->name)
					execute_datapipe_filters(&contact_fill_pipe, msg->contact);
				contacts = g_list_append(contacts, contact_copy(msg->contact));
			} else {
				Contact *contact = g_malloc0(sizeof(*contact));
				contact->line_identifier = g_strdup(msg->line_identifier);
				contact->backend = msg->backend;
				execute_datapipe_filters(&contact_fill_pipe, contact);
				contacts = g_list_append(contacts, contact);
			}
		}
	}
	store_free_message_list(messages);
	return contacts;
}

GList *store_get_all_calls(unsigned int limit)
{
	return store_get_calls_for_contact(NULL, limit);
}

GList *store_get_messages(unsigned int limit)
{
	return store_get_messages_for_contact(NULL, limit);
}

void store_free_call_list(GList *list)
{
	for(GList *element = list; element; element = element->next)
		call_properties_free(element->data);
	g_list_free(list);
}

void store_free_contacts_list(GList *list)
{
	for(GList *element = list; element; element = element->next)
		contact_free(element->data);
	g_list_free(list);
}

void store_free_message_list(GList *list)
{
	for(GList *element = list; element; element = element->next)
		message_properties_free(element->data);
	g_list_free(list);
}

void store_unregister_backend(int id)
{
	(void)id;
}
