#include "storage.h"
#include "sphone-log.h"

GList *(*get_messages_for_contact_backend)(const Contact *contact);
GList *(*get_calls_for_contact_backend)(const Contact *contact);

int store_register_backend(GList *(*get_messages_for_contact)(const Contact *contact),
						   GList *(*get_calls_for_contact)(const Contact *contact))
{
	get_messages_for_contact_backend = get_messages_for_contact;
	get_calls_for_contact_backend = get_calls_for_contact;
	return 0;
}

GList *store_get_messages_for_contact(const Contact *contact)
{
	if(!get_messages_for_contact_backend) {
		sphone_log(LL_ERR, "%s used without backend", __func__);
		return NULL;
	}
	return get_messages_for_contact_backend(contact);
}

GList *store_get_calls_for_contact(const Contact *contact)
{
	if(!get_calls_for_contact_backend) {
		sphone_log(LL_ERR, "%s used without backend", __func__);
		return NULL;
	}
	return get_calls_for_contact_backend(contact);
}

GList * store_get_interacted_msg_contacts(void)
{
	return NULL;
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

void store_free_message_list(GList *list)
{
	for(GList *element = list; element; element = element->next)
		call_properties_free(element->data);
	g_list_free(list);
}

void store_unregister_backend(int id)
{
	(void)id;
}
