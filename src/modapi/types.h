#pragma once

#include <gtk/gtk.h>
#include <time.h>
#include <stdbool.h>

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
	SPHONE_CALL_WATING,
	SPHONE_CALL_DISCONNECTED,
} sphone_call_state_t;

const char *sphone_get_state_string(sphone_call_state_t state);

typedef struct _Contact {
	gchar *name;
	GdkPixbuf *photo;
} Contact;

void contact_free(Contact *contact);

Contact *contact_copy(const Contact *contact);

typedef struct _CallProperties{
	Contact *contact;
	gchar *line_identifier;
	sphone_call_state_t state;
	gchar *technology;
	gint backend;
	gchar *backend_data;
	time_t start_time;
	bool emergency;
	bool awnserd;
	bool needs_route;
} CallProperties;

void call_properties_free(CallProperties *properties);

bool call_properties_comp(const CallProperties *a, const CallProperties *b);

CallProperties *call_properties_copy(const CallProperties *properties);

typedef struct _MessageProperties{
	Contact *contact;
	gchar *line_identifier;
	gchar *technology;
	gchar *text;
	gint backend;
	gchar *backend_data;
	time_t time;
} MessageProperties;

MessageProperties *message_properties_copy(const MessageProperties *properties);

void message_properties_free(MessageProperties *properties);

typedef struct _Notification{
	gchar *title;
	gchar *text;
} Notification;

void notification_free(Notification *notification);

struct str_list {
  char **data;
  int count;
};
