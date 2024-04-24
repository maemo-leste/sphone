/*
 * types.h
 * Copyright (C) Carl Philipp Klemm 2021 <carl@uvos.xyz>
 * 
 * types.h is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * types.h is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <time.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	SPHONE_AUDIO_ROUTE_UNKNOWN=0,
	SPHONE_AUDIO_ROUTE_SPEAKER,
	SPHONE_AUDIO_ROUTE_HANDSET,
	SPHONE_AUDIO_ROUTE_HEADSET,
	SPHONE_AUDIO_ROUTE_BT,
	SPHONE_AUDIO_ROUTE_COUNT
} sphone_audio_route_t;

typedef enum {
	SPHONE_MODE_NO_CALL=0,
	SPHONE_MODE_RINGING,
	SPHONE_MODE_INCALL,
	SPHONE_MODE_INCALL_NO_ROUTE,
} sphone_call_mode_t;

typedef enum {
	SPHONE_VIBRATE_STOP=0,
	SPHONE_VIBRATE_CALL,
	SPHONE_VIBRATE_MESSAGE,
} sphone_vibrate_type_t;

typedef enum {
	SPHONE_CALL_INVALID=0,
	SPHONE_CALL_ACTIVE,
	SPHONE_CALL_HELD,
	SPHONE_CALL_DIALING,
	SPHONE_CALL_ALERTING,
	SPHONE_CALL_INCOMING,
	SPHONE_CALL_WAITING,
	SPHONE_CALL_DISCONNECTED,
} sphone_call_state_t;

typedef enum {
	SPHONE_FIELD_LISTEND = 0,
	SPHONE_FIELD_INVALID = 0,

	SPHONE_FIELD_PHONE,
	SPHONE_FIELD_SIP,

	SPHONE_FIELD_EMAIL,

	SPHONE_FIELD_IM_JABBER,
	SPHONE_FIELD_IM_SKYPE,
	SPHONE_FIELD_IM_GOOGLE_TALK,
	SPHONE_FIELD_IM_TWITTER,
	SPHONE_FIELD_IM_MATRIX,
	SPHONE_FIELD_IM_IRC,
	SPHONE_FIELD_IM_TELEGRAM,
	SPHONE_FIELD_IM_SIGNAL,
	SPHONE_FIELD_IM_WHATSAPP,
	SPHONE_FIELD_IM_OTHER,
} sphone_contact_field_t;

const char *sphone_get_state_string(sphone_call_state_t state);

typedef struct _Contact {
	char *name;
	char *line_identifier;
	sphone_contact_field_t line_identifier_field;
	int backend;
} Contact;

void contact_free(Contact *contact);

Contact *contact_copy(const Contact *contact);

bool contact_cmp(const Contact *a, const Contact *b);

void contact_print(const Contact *contact, const char *module_name);

typedef struct _CallProperties{
	Contact *contact;
	char *line_identifier;
	sphone_call_state_t state;
	int backend;
	char *backend_data;
	time_t start_time;
	time_t end_time;
	bool emergency;
	bool answered;
	bool needs_route;
	bool outbound;
} CallProperties;

void call_properties_print(const CallProperties *call, const char *module_name);

void call_properties_free(CallProperties *properties);

bool call_properties_comp(const CallProperties *a, const CallProperties *b);

CallProperties *call_properties_copy(const CallProperties *properties);

typedef struct _MessageProperties{
	Contact *contact;
	char *line_identifier;
	char *technology;
	char *text;
	int backend;
	char *backend_data;
	time_t time;
	bool outbound;
} MessageProperties;

MessageProperties *message_properties_copy(const MessageProperties *properties);

void message_properties_print(const MessageProperties *call, const char *module_name);

void message_properties_free(MessageProperties *properties);

typedef struct _Notification{
	char *title;
	char *text;
} Notification;

void notification_free(Notification *notification);

struct str_list {
  char **data;
  int count;
};

const Contact *contact_from_message(const MessageProperties *msg);
const Contact *contact_from_call(const CallProperties *call);

#ifdef __cplusplus
}
#endif

