#pragma once

#include <glib.h>

#include "types.h"

GList *store_get_all_calls(void);
GList *store_get_messages(void);

GList *store_get_messages_for_contact(const Contact *contact);
GList *store_get_calls_for_contact(const Contact *contact);
GList *store_get_interacted_msg_contacts(void);

void store_free_call_list(GList *list);
void store_free_message_list(GList *list);

int store_register_backend(GList *(*get_messages_for_contact)(const Contact *contact),
						   GList *(*get_calls_for_contact)(const Contact *contact));

void store_unregister_backend(int id);
