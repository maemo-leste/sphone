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

#ifndef _STORE_H_
#define _STORE_H_

#include <glib-object.h>

#include "types.h"

typedef enum { 
	STORE_INTERACTION_TYPE_MESSAGE,
	STORE_INTERACTION_TYPE_VOICE=1
} store_interaction_type_enum;

typedef enum {
	STORE_INTERACTION_DIRECTION_INCOMING=0,
	STORE_INTERACTION_DIRECTION_OUTGOING=1
} store_interaction_direction_enum;

typedef struct{
	int id;
	store_interaction_type_enum type;
	store_interaction_direction_enum direction;
	union {
		MessageProperties *message;
		CallProperties *call;
	}
} store_interaction_struct;


typedef struct sqlite3_stmt sqlite3_stmt;

int store_init(void);
int store_add(const store_interaction_struct *inter);
int store_contact_load_interactions(store_contact_struct *contact, store_interaction_type_enum type, store_interaction_direction_enum direction, int max_count, time_t from, time_t to);
int store_interactions_get(GPtrArray **contacts, store_interaction_type_enum type, store_interaction_direction_enum direction, int max_count, time_t from, time_t to);
int store_sql_exec(const gchar *sql, sqlite3_stmt **ret_stmt, GType t, ...);
int store_contact_add(const gchar *name);
int store_contact_add_dial(gint contactid, const gchar *dial);
int store_dial_get_id(const gchar *dial);
gchar *store_interaction_sms_get_by_id(gint id);
int store_bulk_transaction_start(void);
int store_transaction_commit(void);
int store_transaction_rollback(void);

#endif
