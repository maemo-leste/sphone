/*
 * sphone
 * Copyright (C) Ahmed Abdel-Hamid 2010 <ahmedam@mail.usa.com>
 * Copyright (C) Carl Philipp Klemm 2021 <carl@uvos.xyz>
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

#ifndef _OFONO_H_
#define _OFONO_H_

typedef struct _OfonoNetworkProperties{
	gchar *status;
	gchar *technology;
	gchar *noperator;
	guint  strength;
}OfonoNetworkProperties;

typedef void (*network_handler)(OfonoNetworkProperties *properties, void *data);

typedef enum{
	CALL_PROPERTY_LINE_ID,
	CALL_PROPERTY_STATE,
	CALL_PROPERTY_START_TIME
} call_property_t;

typedef struct _OfonoCallProperties{
	gchar *path;
	gchar *line_identifier;
	gchar *state;
	gchar *start_time;
	gboolean emergency;
}OfonoCallProperties;

typedef void (*call_handler_t)(const OfonoCallProperties *properties, size_t count, void *data);
typedef void (*call_property_handler_t)(call_property_t property, const gchar *value, void *data);


typedef void (*sms_handler_t)(const gchar *from, const gchar *text, time_t time, void *data);

int ofono_init(void);
int ofono_read_network_properties(OfonoNetworkProperties *properties);
void ofono_network_properties_free(OfonoNetworkProperties *properties);
int ofono_network_properties_add_handler(gpointer handler, gpointer data);
void ofono_network_properties_remove_handler(void);

int ofono_voice_call_add_new_call_handler(call_handler_t handler, void *data);
int ofono_voice_call_get_calls(OfonoCallProperties **calls, size_t *count);

OfonoCallProperties *ofono_call_properties_read(gchar *path);
void ofono_call_properties_free(OfonoCallProperties *properties);
int ofono_voice_call_properties_add_handler(gchar *path, call_property_handler_t handler, void* data);
int ofono_voice_call_properties_remove_handler(int id);
int ofono_call_answer(gchar *path);
int ofono_call_hangup(gchar *path);
int ofono_call_hold_and_answer(void);
int ofono_call_swap(void);
int ofono_dial(const gchar *dial);
int ofono_sms_send(const gchar *to, const gchar *text);
int ofono_sms_incoming_add_handler(sms_handler_t handler, void *data);
void ofono_clear(void);

#endif
