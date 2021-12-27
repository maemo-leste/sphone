/*
 * storage.h
 * Copyright (C) Carl Philipp Klemm 2021 <carl@uvos.xyz>
 * 
 * storage.h is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * storage.h is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <glib.h>

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

GList *store_get_all_calls(void);
GList *store_get_messages(void);

GList *store_get_messages_for_contact(Contact *contact);
GList *store_get_calls_for_contact(Contact *contact);
GList *store_get_interacted_msg_contacts(void);

void store_free_call_list(GList *list);
void store_free_message_list(GList *list);
void store_free_contacts_list(GList *list);

int store_register_backend(GList *(*get_messages_for_contact)(Contact *contact),
						   GList *(*get_calls_for_contact)(Contact *contact));

void store_unregister_backend(int id);

#ifdef __cplusplus
}
#endif
