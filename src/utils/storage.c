#include "storage.h"
#include "sphone-log.h"
#include "datapipes.h"
#include "datapipe.h"

GList *(*get_messages_for_contact_backend)(Contact *contact);
GList *(*get_calls_for_contact_backend)(Contact *contact);

int store_register_backend(GList *(*get_messages_for_contact)(Contact *contact),
						   GList *(*get_calls_for_contact)(Contact *contact))
{
	get_messages_for_contact_backend = get_messages_for_contact;
	get_calls_for_contact_backend = get_calls_for_contact;
	return 0;
}

GList *store_get_messages_for_contact(Contact *contact)
{
	if(!get_messages_for_contact_backend) {
		sphone_log(LL_ERR, "%s used without backend", __func__);
		return NULL;
	}
	return get_messages_for_contact_backend(contact);
}

GList *store_get_calls_for_contact(Contact *contact)
{
	if(!get_calls_for_contact_backend) {
		sphone_log(LL_ERR, "%s used without backend", __func__);
		return NULL;
	}
	return get_calls_for_contact_backend(contact);
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
	GList *messages = store_get_messages();
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

GList *store_get_all_calls(void)
{
	return store_get_calls_for_contact(NULL);
}

GList *store_get_messages(void)
{
	return store_get_messages_for_contact(NULL);
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
